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
#include "Anim.h"
#include "Battlefield.h"
#include "GameState.h"
#include "Pathfinder.h"
#include "Unit.h"
#include "algo.h"
#include "hex_utils.h"
#include "sdl_helper.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
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

namespace
{
    std::shared_ptr<Battlefield> bf;
    SDL_Rect bfWindow = {0, 0, 288, 360};
    std::unordered_map<std::string, int> mapUnitPos;
    std::deque<std::unique_ptr<Anim>> anims;
    bool actionTaken = false;

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
            std::cerr << "Expected top-level object in json file\n";
            return false;
        }

        return true;
    }
}

// Get path from the active unit's hex to the target hex.
std::vector<int> getPathTo(int aTgt)
{
    auto emptyHexes = [&] (int aIndex) {
        std::vector<int> nbrs;
        for (auto n : bf->aryNeighbors(aIndex)) {
            if (!gs->getUnitAt(n)) {
                nbrs.push_back(n);
            }
        }
        return nbrs;
    };

    Pathfinder pf;
    pf.setNeighbors(emptyHexes);
    pf.setGoal(aTgt);
    return pf.getPathFrom(gs->getActiveUnit()->aHex);
}

// Return true if the active unit has a ranged attack and there are no enemies
// adjacent to it.
bool isRangedAttackAllowed()
{
    return false;
    const auto attacker = gs->getActiveUnit();
    if (!attacker || !attacker->type->hasRangedAttack) {
        return false;
    }

    auto enemy = (attacker->team == 0) ? 1 : 0;
    for (auto n : bf->aryNeighbors(attacker->aHex)) {
        const auto defender = gs->getUnitAt(n);
        if (defender && defender->team == enemy) {
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
    const auto attacker = gs->getActiveUnit();
    if (!attacker) {
        return {};
    }

    const auto defender = gs->getUnitAt(aTgt);
    int enemy = (attacker->team == 0) ? 1 : 0;

    // Pathfinder includes the current hex.
    auto moveRange = static_cast<unsigned>(attacker->type->moves) + 1;

    if (defender && defender->team == enemy) {
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
    else if (!defender) {
        auto path = getPathTo(aTgt);
        if (path.size() > 1 && path.size() <= moveRange) {
            return {path, ActionType::MOVE};
        }
    }

    return {};
}

// Get direction to have a unit in the source hex face the target hex.
Facing getFacing(int aSrc, int aTgt, Facing curFacing)
{
    auto hSrc = bf->hexFromAry(aSrc);
    auto hTgt = bf->hexFromAry(aTgt);
    auto dir = direction(hSrc, hTgt);

    if (dir == Dir::NW || dir == Dir::SW) {
        return Facing::LEFT;
    }
    if (dir == Dir::N || dir == Dir::S) {
        return curFacing;
    }

    return Facing::RIGHT;
}

// Make the active unit carry out the given action.
void executeAction(const Action &action)
{
    if (action.type == ActionType::NONE) {
        return;
    }

    auto unit = gs->getActiveUnit();
    if (!unit) {
        return;
    }

    // All other actions might involve moving.
    if (action.path.size() > 1) {
        auto facing = unit->face;
        for (auto i = 1u; i < action.path.size(); ++i) {
            auto aSrc = action.path[i - 1];
            auto aDest = action.path[i];
            auto hDest = bf->hexFromAry(aDest);
            auto prevFacing = facing;
            facing = getFacing(aSrc, aDest, prevFacing);

            anims.emplace_back(make_unique<AnimMove>(*unit, hDest, facing));
        }
        unit->aHex = action.path.back();
        unit->face = facing;
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
        auto attackSeq = make_unique<AnimParallel>();

        auto aSrc = action.path.back();
        auto hSrc = bf->hexFromAry(aSrc);
        auto aTgt = action.attackTarget;
        auto hTgt = bf->hexFromAry(aTgt);

        unit->face = getFacing(aSrc, aTgt, unit->face);
        attackSeq->add(make_unique<AnimAttack>(*unit, hTgt));

        auto defender = gs->getUnitAt(action.attackTarget);
        defender->face = getFacing(aTgt, aSrc, defender->face);
        attackSeq->add(make_unique<AnimDefend>(*defender, hSrc));

        anims.emplace_back(std::move(attackSeq));
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

void handleMouseClick(const SDL_MouseButtonEvent &event)
{
    if (event.button == SDL_BUTTON_LEFT &&
        insideRect(event.x, event.y, bfWindow))
    {
        auto action = getPossibleAction(event.x, event.y);
        if (action.type != ActionType::NONE) {
            executeAction(action);
            bf->clearHighlights();
            bf->deselectHex();
            actionTaken = true;
        }
    }
}

bool parseUnits(const rapidjson::Document &doc)
{
    bool unitAdded = false;
    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "units: skipping id '" << i->name.GetString() <<
                "'\n";
            continue;
        }

        gs->addUnitType(i->name.GetString(), UnitType(i->value));
        unitAdded = true;
    }

    return unitAdded;
}

void parseScenario(const rapidjson::Document &doc)
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
        const auto &bfHex = unitPos[posIdx];

        const auto &json = i->value;

        // Ensure we recognize the unit id.
        std::string name;
        if (json.HasMember("id")) {
            name = json["id"].GetString();
        }
        const auto unitType = gs->getUnitType(name);
        if (!unitType) {
            std::cerr << "scenario: skipping unit with unknown id '" <<
                unitType << "'\n";
            continue;
        }

        Unit newUnit;
        newUnit.team = (posIdx < 7) ? 0 : 1;
        newUnit.aHex = bf->aryFromHex(bfHex);
        newUnit.type = unitType;
        newUnit.face = (newUnit.team == 0) ? Facing::RIGHT : Facing::LEFT;
        if (json.HasMember("num")) {
            newUnit.num = json["num"].GetInt();
        }

        SdlSurface img;
        if (newUnit.team == 0) {
            img = newUnit.type->baseImg[0];
        }
        else {
            img = newUnit.type->reverseImg[1];
        }

        newUnit.entityId = gs->addEntity(bfHex, img, ZOrder::CREATURE);
        gs->addUnit(newUnit);
    }
}

extern "C" int SDL_main(int, char **)  // 2-arg form is required by SDL
{
    if (!sdlInit(288, 360, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

    gs = make_unique<GameState>();
    bf = std::make_shared<Battlefield>(bfWindow);

    rapidjson::Document unitsDoc;
    if (!parseJson("units.json", unitsDoc)) {
        return EXIT_FAILURE;
    }
    if (!parseUnits(unitsDoc)) {
        std::cerr << "Error: no unit definitions loaded" << std::endl;
        return EXIT_FAILURE;
    }

    translateUnitPos();

    rapidjson::Document scenario;
    if (!parseJson("scenario.json", scenario)) {
        return EXIT_FAILURE;
    }
    parseScenario(scenario);

    gs->nextTurn();
    bf->selectHex(gs->getActiveUnit()->aHex);
    bf->draw();

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

            // Ignore mouse events while animating.
            if (!anims.empty()) {
                continue;
            }

            if (event.type == SDL_MOUSEMOTION) {
                handleMouseMotion(event.motion);
                needRedraw = true;
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                handleMouseClick(event.button);
            }
        }

        // Run the current animation.
        if (!anims.empty()) {
            anims.front()->execute();
            if (anims.front()->isDone()) {
                anims.pop_front();
            }
            needRedraw = true;
        }

        // If we're finished running animations for the previous action, start
        // the next turn.
        if (actionTaken && anims.empty()) {
            actionTaken = false;
            gs->nextTurn();
            bf->selectHex(gs->getActiveUnit()->aHex);
        }

        if (needRedraw) {
            bf->draw();
            SDL_UpdateRect(screen, 0, 0, 0, 0);
        }
        SDL_Delay(1);
    }

    // Ensure SDL resources are cleaned up before the subsystems are torn down.
    gs.reset();
    return EXIT_SUCCESS;
}
