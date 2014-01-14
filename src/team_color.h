/*
    Copyright (C) 2013-2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef TEAM_COLOR_H
#define TEAM_COLOR_H

#include "sdl_helper.h"
#include <vector>

// This is an implementation of the team color algorithm from Battle for
// Wesnoth.  We reserve a specific palette of 19 shades of magenta as a
// reference.  Those colors are replaced at runtime with the corresponding
// color for each team.

using ImageSet = std::vector<SdlSurface>;  // store 1 image per team color

// Reference color for each team.  The other 18 shades are offset from this, 14
// darker and 4 lighter.
const std::vector<SDL_Color> teamColors = {{0x2E, 0x41, 0x9B}, // blue
                                           {0xFF, 0, 0},       // red
                                          };
const int numTeams = teamColors.size();

// Apply each team color to create a series of images.
ImageSet applyTeamColors(const SdlSurface &baseImg);

// Return the color used for unit labels.
SDL_Color getLabelColor(int team);

#endif
