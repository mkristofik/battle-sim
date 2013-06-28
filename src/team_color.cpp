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
#include "team_color.h"
#include <cassert>
#include <unordered_map>

namespace
{
    std::unordered_map<Uint32, int> baseColors;
    std::vector<std::vector<Uint32>> allShades;

    void initBaseColors()
    {
        const SDL_Surface *screen = SDL_GetVideoSurface();
        baseColors.emplace(SDL_MapRGB(screen->format, 0x3F, 0, 0x16), 0);
        baseColors.emplace(SDL_MapRGB(screen->format, 0x55, 0, 0x2A), 1);
        baseColors.emplace(SDL_MapRGB(screen->format, 0x69, 0, 0x39), 2);
        baseColors.emplace(SDL_MapRGB(screen->format, 0x7B, 0, 0x45), 3);
        baseColors.emplace(SDL_MapRGB(screen->format, 0x8C, 0, 0x51), 4);
        baseColors.emplace(SDL_MapRGB(screen->format, 0x9E, 0, 0x5D), 5);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xB1, 0, 0x69), 6);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xC3, 0, 0x74), 7);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xD6, 0, 0x7F), 8);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xEC, 0, 0x8C), 9);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xEE, 0x3D, 0x96), 10);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xEF, 0x5B, 0xA1), 11);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xF1, 0x72, 0xAC), 12);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xF2, 0x87, 0xB6), 13);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xF4, 0x9A, 0xC1), 14);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xF6, 0xAD, 0xCD), 15);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xF8, 0xC1, 0xD9), 16);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xFA, 0xD5, 0xE5), 17);
        baseColors.emplace(SDL_MapRGB(screen->format, 0xFD, 0xE9, 0xF1), 18);
    }

    std::vector<Uint32> makeTeamColors(const SDL_Color &refColor)
    {
        std::vector<Uint32> colors(19);
        const SDL_PixelFormat *format = SDL_GetVideoSurface()->format;

        // Reference color.
        colors[14] = SDL_MapRGB(format, refColor.r, refColor.g, refColor.b);

        // Fade toward black in 1/16th increments.
        auto rStep = refColor.r / 16.0;
        auto gStep = refColor.g / 16.0;
        auto bStep = refColor.b / 16.0;
        double curR = refColor.r;
        double curG = refColor.g;
        double curB = refColor.b;
        for (int i = 13; i >= 0; --i) {
            curR -= rStep;
            curG -= gStep;
            curB -= bStep;
            colors[i] = SDL_MapRGB(format, curR, curG, curB);
        }

        // Fade toward white in 1/5th increments.
        rStep = (255 - refColor.r) / 5.0;
        gStep = (255 - refColor.g) / 5.0;
        bStep = (255 - refColor.b) / 5.0;
        curR = refColor.r;
        curG = refColor.g;
        curB = refColor.b;
        for (int i = 15; i < 19; ++i) {
            curR += rStep;
            curG += gStep;
            curB += bStep;
            colors[i] = SDL_MapRGB(format, curR, curG, curB);
        }

        return colors;
    }

    void initColors()
    {
        if (baseColors.empty()) {
            initBaseColors();
        }
        if (allShades.empty()) {
            for (const auto &color : teamColors) {
                allShades.emplace_back(makeTeamColors(color));
            }
        }
    }

    // Modify an image in place with the given team color.
    // source: Battle for Wesnoth, recolor_image() in sdl_utils.cpp.
    void applyColor(SdlSurface &img, int team)
    {
        assert(team >= 0 && team < numTeams);

        SdlLock(img, [&] {
            auto pixel = static_cast<Uint32 *>(img->pixels);
            auto end = pixel + img->w * img->h;
            for (; pixel != end; ++pixel) {
                Uint32 alpha = (*pixel) & 0xFF000000;
                if (alpha == 0) continue;  // skip invisible pixels

                // Base team colors are RGB only, mask off alpha channel.
                Uint32 curColor = (*pixel) & 0x00FFFFFF;
                const auto &colorKey = baseColors.find(curColor);
                if (colorKey != std::end(baseColors)) {
                    *pixel = alpha + allShades[team][colorKey->second];
                }
            }
        });
    }
}

ImageSet applyTeamColors(const SdlSurface &baseImg)
{
    initColors();

    ImageSet ret;
    for (int i = 0; i < numTeams; ++i) {
        ret.emplace_back(sdlDeepCopy(baseImg));
        applyColor(ret[i], i);
    }

    return ret;
}
