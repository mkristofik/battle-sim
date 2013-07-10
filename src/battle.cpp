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

// TODO: determining which highlight to use:
// - if active unit has the ranged attack trait
//      * and mouseover hex is an enemy more than 1 hex away
//          + draw red hex on target
//      * if mouseover hex is an adjacent enemy
//          + draw appropriate attack arrow on active unit
// - else
//      * if mouseover hex is an enemy within move distance + 1
//          + draw appropriate attack arrow on hex to move to
//          + this might be the hex unit is standing in
//      * if mouseover hex is empty within move distance
//          + draw green hex
//          + dead units count as empty hexes

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

enum class ActionType {NONE, MOVE, ATTACK, RANGED};
struct Action
{
    std::vector<int> path;
    int attackTarget;
    ActionType type;

    Action() : path{}, attackTarget{-1}, type{ActionType::NONE} {}
    Action(std::vector<int> movePath)
        : path{std::move(movePath)},
        attackTarget{-1},
        type{ActionType::MOVE}
    {
    }
    Action(std::vector<int> movePath, int aTgt, ActionType at)
        : path{std::move(movePath)},
        attackTarget{aTgt},
        type{at}
    {
    }
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
    auto path = pf.getPathFrom(stackList[activeUnit].aHex);
    return path;
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
    const auto tgtUnit = getUnitAt(aTgt);

    int myTeam = stackList[activeUnit].team;
    int theirTeam = (myTeam == 0) ? 1 : 0;

    if (tgtUnit && tgtUnit->team == theirTeam) {  // try to attack
        auto pOffset = Point{px, py} - pixelFromHex(hTgt);
        auto attackDir = getSector(pOffset.x, pOffset.y);
        auto hMoveTo = adjacent(hTgt, attackDir);
        auto aMoveTo = bf->aryFromHex(hMoveTo);
        auto path = getPathTo(aMoveTo);
        if (!path.empty() && path.size() <= 4) {  // TODO: use real move distance
            return {path, aTgt, ActionType::ATTACK};
        }
    }
    else {  // move
        auto path = getPathTo(aTgt);
        // path includes the starting hex
        if (path.size() > 1 && path.size() <= 4) {  // TODO: use real move distance
            return {path};
        }
    }

    return {};
}

void handleMouseMotion(const SDL_MouseMotionEvent &event)
{
    bf->clearHighlights();

    if (!insideRect(event.x, event.y, bfWindow)) {
        return;
    }

    auto action = getPossibleAction(event.x, event.y);
    if (action.type == ActionType::ATTACK) {
        auto aMoveTo = action.path.back();
        bf->showMouseover(aMoveTo);
        bf->showAttackArrow2(aMoveTo, action.attackTarget);
    }
    else if (action.type == ActionType::MOVE) {
        auto aMoveTo = action.path.back();
        bf->showMouseover(aMoveTo);
        bf->setMoveTarget(aMoveTo);
    }
    else {
        bf->showMouseover(bf->aryFromPixel(event.x, event.y));
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
    int firstAttacker = stackList[0].entityId;
    bf->selectEntity(firstAttacker);

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
