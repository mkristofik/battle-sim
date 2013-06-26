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
#include "hex_utils.h"
#include "sdl_helper.h"
#include "team_color.h"

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Unit
{
    std::string name;
    std::string plural;
    ImageSet img;
    ImageSet reverseImg;
    ImageSet animAttack;
    ImageSet reverseAnimAttack;
    int numAttackFrames;
    ImageSet imgDefend;
    ImageSet reverseImgDefend;
};

namespace
{
    SdlSurface tile;
    SdlSurface grid;
    std::unordered_map<std::string, Unit> allUnits;
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

    // TODO: notes on where to place units
    // team 1 in upper left, team 2 in lower right
    // team 1 row 1: (3,0), (2,1), (1,1), (0,2)
    // team 1 row 2: (2,0), (1,0), (0,1)
    // team 2 row 1: (4,2), (3,2), (2,3), (1,3)
    // team 2 row 2: (4,3), (3,3), (2,4)
}

void loadUnit(const std::string &id, const rapidjson::Value &json)
{
    Unit u;
    if (json.HasMember("name")) {
        u.name = json["name"].GetString();
    }
    if (json.HasMember("plural")) {
        u.plural = json["plural"].GetString();
    }
    if (json.HasMember("img")) {
        auto baseImg = sdlLoadImage(json["img"].GetString());
        if (baseImg) {
            u.img = applyTeamColors(baseImg);
            u.reverseImg = applyTeamColors(sdlFlipH(baseImg));
        }
    }
    if (json.HasMember("img-defend")) {
        auto baseImg = sdlLoadImage(json["img-defend"].GetString());
        if (baseImg) {
            u.imgDefend = applyTeamColors(baseImg);
            u.reverseImgDefend = applyTeamColors(sdlFlipH(baseImg));
        }
    }
    if (json.HasMember("anim-attack")) {
        auto baseAnim = sdlLoadImage(json["anim-attack"].GetString());
        if (baseAnim) {
            u.animAttack = applyTeamColors(baseAnim);

            // Assume each animation frame is sized to fit the standard hex.
            int n = baseAnim->w / pHexSize;
            u.numAttackFrames = n;
            u.reverseAnimAttack = applyTeamColors(sdlFlipSheetH(baseAnim, n));
        }
    }
    if (json.HasMember("anim-die")) {
    }

    allUnits.emplace(id, std::move(u));
}

bool parseJson()
{
    std::shared_ptr<FILE> fp{fopen("../data/units.json", "r"), fclose};
    rapidjson::Document doc;
    rapidjson::FileStream file(fp.get());
    if (doc.ParseStream<0>(file).HasParseError()) {
        std::cerr << "Error reading units: " << doc.GetParseError() << '\n';
        return false;
    }
    if (!doc.IsObject()) {
        std::cerr << "Expected top-level object in units file\n";
        return false;
    }

    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "units: skipping id '" << i->name.GetString() <<
                "'\n";
            continue;
        }
        loadUnit(i->name.GetString(), i->value);
    }

    return true;
}

extern "C" int SDL_main(int, char **)  // 2-arg form is required by SDL
{
    if (!sdlInit(288, 360, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

    if (!parseJson()) {
        return EXIT_FAILURE;
    }

    loadImages();
    drawMap();

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
