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
#ifndef COMMANDER_VIEW_H
#define COMMANDER_VIEW_H

#include "sdl_helper.h"
class GameState;

class CommanderView
{
public:
    CommanderView(SDL_Rect dispArea, int team, const GameState &gs);
    void draw() const;

private:
    const GameState &gs_;
    int team_;
    const SdlFont &font_;
    SDL_Rect displayArea_;
    Justify txtAlign_;
};

#endif
