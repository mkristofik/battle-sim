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

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    SdlSurface tile;
    SdlSurface grid;
    std::unordered_map<std::string, int> mapUnitPos;

    // Unit placement on the grid.
    // team 1 in upper left, team 2 in lower right
    const Point unitPos[] = {{3,0}, {2,1}, {1,1}, {0,2},  // team 1 row 1
                             {2,0}, {1,0}, {0,1},         // team 1 row 2
                             {4,2}, {3,2}, {2,3}, {1,3},  // team 2 row 1
                             {4,3}, {3,3}, {2,4}};        // team 2 row 2
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

extern "C" int SDL_main(int, char **)  // 2-arg form is required by SDL
{
    if (!sdlInit(288, 360, "icon.png", "Battle Sim")) {
        return EXIT_FAILURE;
    }

    auto unitsMap = parseUnitsJson();
    if (unitsMap.empty()) {
        return EXIT_FAILURE;
    }

    translateUnitPos();
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
