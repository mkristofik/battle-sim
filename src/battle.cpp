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

// TODO: load scenario, define entities
// an entity might have any or all of:
// - a hex position on the map
// - drawing stuff (possible images, offset from hex position)
// - combat stats
// - spellcasting abilities
// - other special abilities
// an entity by itself is just an id

struct Drawable
{
    Point hex;
    Point offset;
    SdlSurface img;
};

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
    SdlSurface tile;
    SdlSurface grid;
    std::unordered_map<std::string, int> mapUnitPos;
    std::vector<Drawable> entities;

    // Unit placement on the grid.
    // team 1 on the left, team 2 on the right
    const Point unitPos[] = {{1,0}, {1,1}, {1,2}, {1,3},  // team 1 row 1
                             {0,1}, {0,2}, {0,3},         // team 1 row 2
                             {3,0}, {3,1}, {3,2}, {3,3},  // team 2 row 1
                             {4,1}, {4,2}, {4,3}};        // team 2 row 2
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

void translateUnitPos()
{
    // Map unit position strings used by JSON data to array indexes.
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

void parseScenario(const rapidjson::Document &doc, const UnitsMap &allUnits)
{
    // TODO: for each object in the scenario
    // - look up its unit position, skip if invalid
    // - if unit position < 7, team 1, otherwise team 2
    // - create a Drawable entity for it
    // - for now, assume we won't ever have an entity that isn't drawn
    // - build a unit stack for it
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
        auto battlefieldHex = unitPos[posIdx];

        const auto &json = i->value;

        // Ensure we recognize the unit id.
        std::string unitType;
        if (json.HasMember("id")) {
            unitType = json["id"].GetString();
        }
        auto unitIter = allUnits.find(unitType);
        if (unitIter == std::end(allUnits)) {
            std::cerr << "scenario: skipping unit with unknown id '" <<
                unitType << "'\n";
            continue;
        }

        UnitStack u;
        u.entityId = entities.size();
        u.team = (posIdx < 7) ? 0 : 1;
        u.unitDef = &unitIter->second;
        if (json.HasMember("num")) {
            u.num = json["num"].GetInt();
        }

        Drawable entity;
        entity.hex = battlefieldHex;
        entity.offset = {0, 0};
        if (u.team == 0) {
            entity.img = u.unitDef->baseImg[0];
        }
        else {
            entity.img = u.unitDef->reverseImg[1];
        }

        entities.emplace_back(entity);
    }
}

void loadImages()
{
    tile = sdlLoadImage("../img/grass.png");
    grid = sdlLoadImage("../img/hex-grid.png");
}

void drawMap()
{
    // Background layer.
    for (int tileX = -1; tileX <= 5; ++tileX) {
        for (int tileY = -1; tileY <= 5; ++tileY) {
            sdlBlit(tile, pixelFromHex(tileX, tileY));
        }
    }

    // 5-hex wide hexagonal grid (draw a square, cut off the corners).
    for (int gridX = 0; gridX < 5; ++gridX) {
        for (int gridY = 1; gridY < 4; ++gridY) {
            sdlBlit(grid, pixelFromHex(gridX, gridY));
        }
    }
    sdlBlit(grid, pixelFromHex(1, 0));
    sdlBlit(grid, pixelFromHex(2, 0));
    sdlBlit(grid, pixelFromHex(3, 0));
    sdlBlit(grid, pixelFromHex(2, 4));
}

void drawUnits()
{
    for (const auto &e : entities) {
        auto pos = pixelFromHex(e.hex) + e.offset;
        sdlBlit(e.img, pos);
    }
}

extern "C" int SDL_main(int, char **)  // 2-arg form is required by SDL
{
    if (!sdlInit(288, 360, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

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

    loadImages();
    drawMap();
    drawUnits();

    SDL_Surface *screen = SDL_GetVideoSurface();
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    bool isDone = false;
    SDL_Event event;
    while (!isDone) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isDone = true;
            }
        }
        SDL_Delay(1);
    }

    return EXIT_SUCCESS;
}
