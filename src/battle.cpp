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

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"

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
}

void loadUnitImage(const std::string &file,
                   ImageSet &images, ImageSet &reversed)
{
    namespace bfs = boost::filesystem;
    const bfs::path imagePath{"../img"};

    auto p = imagePath;
    p /= file;
    auto baseImg = sdlLoadImage(p.string());
    if (baseImg) {
        images = applyTeamColors(baseImg);
        reversed = applyTeamColors(sdlFlipH(baseImg));
    }
    else {
        std::cerr << "Error loading unit image: " << p.string() << '\n';
    }
}

void loadUnitAnim(const std::string &file, ImageSet &anims, ImageSet &reversed,
                  int &numFrames)
{
    namespace bfs = boost::filesystem;
    const bfs::path imagePath{"../img"};

    auto p = imagePath;
    p /= file;
    auto baseAnim = sdlLoadImage(p.string());
    if (!baseAnim) {
        std::cerr << "Error loading unit image: " << p.string() << '\n';
        return;
    }

    anims = applyTeamColors(baseAnim);

    // Assume each animation frame is sized to fit the standard hex.
    numFrames = baseAnim->w / pHexSize;
    reversed = applyTeamColors(sdlFlipSheetH(baseAnim, numFrames));
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
        loadUnitImage(json["img"].GetString(), u.img, u.reverseImg);
    }
    if (json.HasMember("img-defend")) {
        loadUnitImage(json["img-defend"].GetString(), u.imgDefend,
                      u.reverseImgDefend);
    }
    if (json.HasMember("anim-attack")) {
        loadUnitAnim(json["anim-attack"].GetString(), u.animAttack,
                     u.reverseAnimAttack, u.numAttackFrames);
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
    if (!sdlInit(288, 360, "../img/icon.png", "Battle Sim")) {
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
