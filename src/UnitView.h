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
class Unit;

/*
 * -------
 * |     |  23 Archers
 * |     |  Dmg 2-3
 * |     |  HP 8/10
 * -------
 *  Trait 1, Trait 2, ...
 *  Status 1, Status 2, ...
 */

class UnitView
{
public:
    UnitView(SDL_Rect dispArea, int team, const GameState &gs, const SdlFont &f);
    void draw() const;

private:
    SdlSurface renderName(const Unit &unit) const;
    SdlSurface renderDamage(const Unit &unit) const;
    SdlSurface renderHP(const Unit &unit) const;
    SdlSurface renderTraits(const Unit &unit) const;
    SdlSurface renderEffects(const Unit &unit) const;

    SDL_Rect displayArea_;
    int team_;
    const GameState &gs_;
    const SdlFont &font_;
};

#endif
