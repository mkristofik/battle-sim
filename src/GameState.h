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
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "Drawable.h"
#include "sdl_helper.h"

#include <memory>
#include <vector>
// I think we want this to be a library of functions that grant access to all
// the entities on the battlefield, their stats, and other info.  Behaves like
// global variables to but access is through functions, all keyed off entity
// id.

class GameState
{
public:
    // Add a drawable entity to the battlefield.  Return its unique id number.
    int addEntity(Point hex, SdlSurface img, ZOrder z);
    int addHiddenEntity(SdlSurface img, ZOrder z);

    int numEntities() const;
    Drawable & getEntity(int id);

private:
    std::vector<Drawable> entities_;
};

extern std::unique_ptr<GameState> gs;

#endif
