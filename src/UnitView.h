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
#ifndef UNIT_VIEW_H
#define UNIT_VIEW_H

#include "sdl_helper.h"

class GameState;

class UnitView
{
public:
    UnitView(SDL_Rect dispArea, int team, const GameState &gs, const SdlFont &f);
    void draw() const;

private:
    SDL_Rect displayArea_;
    int team_;
    const GameState &gs_;
    const SdlFont &font_;
};

#endif
