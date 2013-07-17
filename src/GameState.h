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
#include "Unit.h"
#include "sdl_helper.h"

#include "rapidjson/document.h"

#include <memory>
#include <vector>

class GameState
{
public:
    GameState();

    void nextTurn();

    // Add a drawable entity to the battlefield.  Return its unique id number.
    int addEntity(Point hex, SdlSurface img, ZOrder z);
    int addHiddenEntity(SdlSurface img, ZOrder z);

    int numEntities() const;
    Drawable & getEntity(int id);

    // The set of available units.
    void addUnit(std::string name, Unit u);
    const Unit * getUnit(const std::string &name) const;

    // Troop stacks on the battlefield.
    void addTroop(Troop t);
    Troop * getActiveTroop();
    Troop * getTroopAt(int aIndex);

private:
    void parseUnits(const rapidjson::Document &doc);

    std::vector<Drawable> entities_;
    UnitsMap unitRef_;
    std::vector<Troop> troops_;
    int activeTroop_;
};

extern std::unique_ptr<GameState> gs;

#endif
