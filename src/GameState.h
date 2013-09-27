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
#include "UnitType.h"
#include "sdl_helper.h"

#include "rapidjson/document.h"

#include <memory>
#include <vector>

enum class Winner {NOBODY_YET = -1, DRAW, TEAM_1, TEAM_2};

class GameState
{
public:
    GameState();

    void nextTurn();
    int getRound() const;
    Winner getWinner() const;

    // Add a drawable entity to the battlefield.  Return its unique id number.
    int addEntity(Point hex, SdlSurface img, ZOrder z);
    int addHiddenEntity(SdlSurface img, ZOrder z);

    int numEntities() const;

    // References might get invalidated at any time.  Recommend calling this
    // instead of holding on to entity object refs for too long.
    Drawable & getEntity(int id);

    // The set of available unit types.
    void addUnitType(std::string name, UnitType u);
    const UnitType * getUnitType(const std::string &name) const;

    // Unit stacks on the battlefield.
    void addUnit(Unit u);
    Unit * getActiveUnit();
    Unit * getUnitAt(int aIndex);

private:
    std::vector<Drawable> entities_;
    UnitTypeMap unitRef_;
    std::vector<Unit> bfUnits_;
    int activeUnit_;
    int roundNum_;
};

extern std::unique_ptr<GameState> gs;

#endif
