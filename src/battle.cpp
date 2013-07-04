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
#include "Unit.h"
#include "hex_utils.h"
#include "sdl_helper.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"
#include "boost/tokenizer.hpp"

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: ideas
// - make a bigger window with some portraits to indicate heroes
//      * need some scaling because the portraits are big
// - leave space to write unit stats for the highlighted hex
// - we'll need hex graphics:
//      * attack arrows
//      * red for a ranged attack target

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
// - which unit is standing in the given hex?
// - traits (X macros?)
// - path to mouseover hex
//      * a hex is unwalkable if it has an alive unit in it, unless it's the
//        target

struct UnitStack
{
    int entityId;
    int num;
    int team;
    const Unit *unitDef;

    UnitStack() : entityId{-1}, num{0}, team{-1}, unitDef{nullptr} {}
};

namespace
{
    std::shared_ptr<Battlefield> bf;
    SDL_Rect bfWindow = {0, 0, 288, 360};
    std::unordered_map<std::string, int> mapUnitPos;
    std::vector<Drawable> entities;
    std::vector<UnitStack> stackList;

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

bool isUnitAt(const Point &hex)
{
    for (const auto &s : stackList) {
        if (bf->getEntity(s.entityId).hex == hex) {
            return true;
        }
    }

    return false;
}

void handleMouseMotion(const SDL_MouseMotionEvent &event)
{
    if (insideRect(event.x, event.y, bfWindow)) {
        bf->showMouseover(event.x, event.y);

        auto hex = bf->hexFromPixel(event.x, event.y);
        auto curHex = bf->getEntity(stackList[0].entityId).hex;
        if (bf->isHexValid(hex) && hexDist(hex, curHex) == 1 && !isUnitAt(hex))
        {
            bf->setMoveTarget(hex);
        }
        else {
            bf->clearMoveTarget();
        }
    }
    else {
        bf->hideMouseover();
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

std::vector<UnitStack> parseScenario(const rapidjson::Document &doc, const UnitsMap &unitReference)
{
    std::vector<UnitStack> unitStacks;

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
        unitStacks.emplace_back(std::move(u));
    }

    return unitStacks;
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
    stackList = parseScenario(scenario, unitsMap);

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
