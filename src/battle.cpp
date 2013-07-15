/*
    Copyright (C) 2013 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "Action.h"
#include "Battlefield.h"
#include "Pathfinder.h"
#include "Unit.h"
#include "hex_utils.h"
#include "sdl_helper.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"
#include "boost/tokenizer.hpp"

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: ideas
// - make a bigger window with some portraits to indicate heroes
//      * need some scaling because the portraits are big
// - leave space to write unit stats for the highlighted hex

// TODO: things we need to figure all this out
// - traits (X macros?)

struct UnitStack
{
    int entityId;
    int num;
    int team;
    int aHex;
    const Unit *unitDef;

    UnitStack() : entityId{-1}, num{0}, team{-1}, aHex{-1}, unitDef{nullptr} {}
};

namespace
{
    std::shared_ptr<Battlefield> bf;
    SDL_Rect bfWindow = {0, 0, 288, 360};
    std::unordered_map<std::string, int> mapUnitPos;
    std::vector<UnitStack> stackList;
    int activeUnit = 0;

    // Unit placement on the grid.
    // team 1 on the left, team 2 on the right
    const Point unitPos[] = {{1,0}, {1,1}, {1,2}, {1,3},  // team 1 row 1
                             {0,1}, {0,2}, {0,3},         // team 1 row 2
                             {3,0}, {3,1}, {3,2}, {3,3},  // team 2 row 1
                             {4,1}, {4,2}, {4,3}};        // team 2 row 2

    // Map unit position strings used by JSON data to 'unitPos' array indexes.
    void translateUnitPos()
    {
        mapUnitPos.emplace("t1p1", 0);
        mapUnitPos.emplace("t1p2", 1);
        mapUnitPos.emplace("t1p3", 2);
        mapUnitPos.emplace("t1p4", 3);
        mapUnitPos.emplace("t1p5", 4);
        mapUnitPos.emplace("t1p6", 5);
        mapUnitPos.emplace("t1p7", 6);
        mapUnitPos.emplace("t2p1", 7);
        mapUnitPos.emplace("t2p2", 8);
        mapUnitPos.emplace("t2p3", 9);
        mapUnitPos.emplace("t2p4", 10);
        mapUnitPos.emplace("t2p5", 11);
        mapUnitPos.emplace("t2p6", 12);
        mapUnitPos.emplace("t2p7", 13);
    }
}

// Return the live unit at the given hex, if any.
const UnitStack * getUnitAt(int aIndex)
{
    for (const auto &s : stackList) {
        if (s.aHex == aIndex && s.num > 0) {
            return &s;
        }
    }

    return nullptr;
}

const UnitStack & getActiveUnit()
{
    return stackList[activeUnit];
}

// Get path from the active unit to the target hex.
std::vector<int> getPathTo(int aTgt)
{
    auto emptyHexes = [&] (int aIndex) {
        std::vector<int> nbrs;
        for (auto n : bf->aryNeighbors(aIndex)) {
            const auto unit = getUnitAt(n);
            if (!unit || unit->num == 0) {
                nbrs.push_back(n);
            }
        }
        return nbrs;
    };

    Pathfinder pf;
    pf.setNeighbors(emptyHexes);
    pf.setGoal(aTgt);
    return pf.getPathFrom(getActiveUnit().aHex);
}

// Return true if the active unit has a ranged attack and there are no enemy
// units adjacent to it.
bool isRangedAttackAllowed()
{
    const auto &myUnit = getActiveUnit();
    if (!myUnit.unitDef->hasRangedAttack) {
        return false;
    }

    auto enemy = (myUnit.team == 0) ? 1 : 0;
    for (auto n : bf->aryNeighbors(myUnit.aHex)) {
        const auto otherUnit = getUnitAt(n);
        if (otherUnit && otherUnit->team == enemy) {
            return false;
        }
    }

    return true;
}

// Human player's function - determine what action the active unit can take if
// the user clicks at the given screen coordinates.
Action getPossibleAction(int px, int py)
{
    auto aTgt = bf->aryFromPixel(px, py);
    if (aTgt < 0) {
        return {};
    }
    auto hTgt = bf->hexFromAry(aTgt);
    const auto &myUnit = getActiveUnit();

    const auto tgtUnit = getUnitAt(aTgt);
    int enemy = (myUnit.team == 0) ? 1 : 0;

    // Pathfinder includes the current hex.
    auto moveRange = static_cast<unsigned>(myUnit.unitDef->moves) + 1;

    if (tgtUnit && tgtUnit->team == enemy) {
        if (isRangedAttackAllowed()) {
            return {{}, ActionType::RANGED, aTgt};
        }

        auto pOffset = Point{px, py} - pixelFromHex(hTgt);
        auto attackDir = getSector(pOffset.x, pOffset.y);
        auto hMoveTo = adjacent(hTgt, attackDir);
        auto aMoveTo = bf->aryFromHex(hMoveTo);
        auto path = getPathTo(aMoveTo);
        if (!path.empty() && path.size() <= moveRange) {
            return {path, ActionType::ATTACK, aTgt};
        }
    }
    else {
        auto path = getPathTo(aTgt);
        if (path.size() > 1 && path.size() <= moveRange) {
            return {path, ActionType::MOVE};
        }
    }

    return {};
}

// Return true if target hex is left of the given unit.
bool useReverseImg(const UnitStack &unit, int aTgt)
{
    auto hSrc = bf->hexFromAry(unit.aHex);
    auto hTgt = bf->hexFromAry(aTgt);
    auto dir = direction(hSrc, hTgt);

    if (dir == Dir::NW || dir == Dir::SW) {
        return true;
    }

    // Team 0 always starts facing right and team 1 faces left.
    if ((dir == Dir::N || dir == Dir::S) && unit.team == 1) {
        return true;
    }

    return false;
}

// Make the active unit carry out the given action.
void executeAction(const Action &action)
{
    if (action.type == ActionType::NONE) {
        return;
    }

    // All other actions might involve moving.
    if (action.path.size() > 1) {
        // TODO: animate the moves to each hex, first element of path is
        // starting hex
        //
        // start animation:
        // - facing left or right?
        // - set the attacker image to 'moving'
        // - set the attacker zorder to ANIMATING
        // over the course of 300 ms:
        // - adjust attacker pOffset toward destination hex
        // end animation:
        // - set attacker image to 'base'
        // - set attacker zorder to CREATURE
    }

    // These animations happen after the attacker is done moving.
    if (action.type == ActionType::RANGED) {
        // TODO: animate the attacker, projectile, and defender
        // need to run animations in parallel
        //
        // shooter:
        // - start animation:
        //      * facing left or right?
        // - during animation:
        //      * set attacker frame per sequence
        //      * play bow sound if time reached and sound not yet played
        //      * animation is done if we've passed the last frame
        // - end animation:
        //      * set attacker image to 'base'
        //
        // projectile:
        // - start animation:
        //      * determine which angle we need to use
        //      * create entity at shooter's hex, zorder PROJECTILE, invisible
        // - during animation:
        //      * if shot time not reached, do nothing
        //      * make visible
        //      * slide pOffset toward target hex at 150 ms per hex
        //      * if target hex reached, we're done
        // - end animation:
        //      * hide entity
        //
        // defender:
        // - start:
        //      * facing left or right?
        //      * get time of flight plus shot time
        // - during:
        //      * set defend image if hit time reached
        //      * play hit sound if not played yet
        //      * lasts for 250 ms
        // - end:
        //      * set defender image to 'base'
    }
    else if (action.type == ActionType::ATTACK) {
        // animate attacker and defender in parallel
        //
        // attacker:
        // - start:
        //      * facing left or right?
        // - during:
        //      * set attacker frame per sequence
        //      * play attack sound if time reached and sound not yet played
        //      * slide toward target for 300 ms, slide back for 300 ms
        //        (previous demo has code for this)
        //      * done when sliding done or last frame reached, whichever is
        //        later
        // - end:
        //      * set attacker image to 'base'
        //
        // defender:
        // - start:
        //      * facing left or right?
        //      * compute hit time
        // - during:
        //      * if hit time reached, set defend image for 250 ms
        //      * play hit sound if not yet played
        // - end:
        //      * set defender image to 'base'
    }
}

void handleMouseMotion(const SDL_MouseMotionEvent &event)
{
    if (insideRect(event.x, event.y, bfWindow)) {
        bf->handleMouseMotion(event, getPossibleAction(event.x, event.y));
    }
}

bool parseJson(const char *filename, rapidjson::Document &doc)
{
    boost::filesystem::path dataPath{"../data"};
    dataPath /= filename;
    std::shared_ptr<FILE> fp{fopen(dataPath.string().c_str(), "r"), fclose};

    rapidjson::FileStream file(fp.get());
    if (doc.ParseStream<0>(file).HasParseError()) {
        std::cerr << "Error reading json file " << dataPath.c_str() << ": "
            << doc.GetParseError() << '\n';
        return false;
    }
    if (!doc.IsObject()) {
        std::cerr << "Expected top-level object in units file\n";
        return false;
    }

    return true;
}

void parseScenario(const rapidjson::Document &doc, const UnitsMap &unitReference)
{
    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "scenario: skipping unit at position '"
                << i->name.GetString() << "'\n";
            continue;
        }

        // Compute battlefield position from location id.
        std::string posStr = i->name.GetString();
        auto posIter = mapUnitPos.find(posStr);
        if (posIter == std::end(mapUnitPos)) {
            std::cerr << "scenario: skipping unit at position '"
                << i->name.GetString() << "'\n";
            continue;
        }
        int posIdx = posIter->second;
        const auto &battlefieldHex = unitPos[posIdx];

        const auto &json = i->value;

        // Ensure we recognize the unit id.
        std::string unitType;
        if (json.HasMember("id")) {
            unitType = json["id"].GetString();
        }
        auto unitIter = unitReference.find(unitType);
        if (unitIter == std::end(unitReference)) {
            std::cerr << "scenario: skipping unit with unknown id '" <<
                unitType << "'\n";
            continue;
        }

        UnitStack u;
        u.team = (posIdx < 7) ? 0 : 1;
        u.aHex = bf->aryFromHex(battlefieldHex);
        u.unitDef = &unitIter->second;
        if (json.HasMember("num")) {
            u.num = json["num"].GetInt();
        }

        SdlSurface img;
        if (u.team == 0) {
            img = u.unitDef->baseImg[0];
        }
        else {
            img = u.unitDef->reverseImg[1];
        }

        u.entityId = bf->addEntity(battlefieldHex, img, ZOrder::CREATURE);
        stackList.emplace_back(std::move(u));
    }
}

extern "C" int SDL_main(int, char **)  // 2-arg form is required by SDL
{
    if (!sdlInit(288, 360, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }
    bf = std::make_shared<Battlefield>(bfWindow);

    rapidjson::Document unitsDoc;
    if (!parseJson("units.json", unitsDoc)) {
        return EXIT_FAILURE;
    }
    auto unitsMap = parseUnits(unitsDoc);
    if (unitsMap.empty()) {
        return EXIT_FAILURE;
    }

    translateUnitPos();

    rapidjson::Document scenario;
    if (!parseJson("scenario.json", scenario)) {
        return EXIT_FAILURE;
    }
    parseScenario(scenario, unitsMap);

    bf->draw();
    bf->selectEntity(getActiveUnit().entityId);

    SDL_Surface *screen = SDL_GetVideoSurface();
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    bool isDone = false;
    bool needRedraw = false;
    SDL_Event event;
    while (!isDone) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isDone = true;
            }
            else if (event.type == SDL_MOUSEMOTION) {
                handleMouseMotion(event.motion);
                needRedraw = true;
            }
        }

        if (needRedraw) {
            bf->draw();
            SDL_UpdateRect(screen, 0, 0, 0, 0);
        }
        SDL_Delay(1);
    }

    return EXIT_SUCCESS;
}
