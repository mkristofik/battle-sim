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

#include "Commander.h"
#include "Unit.h"
#include "sdl_helper.h"

#include <utility>
#include <vector>

class GameState
{
public:
    GameState();

    void nextTurn();
    int getRound() const;

    // Unit stacks on the battlefield.
    void addUnit(Unit u);
    Unit * getActiveUnit();
    Unit * getUnitAt(int aIndex);

    // Score the current battle state for each side.  Normalize each unit by
    // comparing size to growth rate.
    std::pair<int, int> getScore() const;

    // Leaders of each army.
    void setCommander(Commander c, int team);
    Commander & getCommander(int team);

private:
    void nextRound();

    std::vector<Unit> bfUnits_;
    std::vector<Unit>::iterator activeUnit_;
    std::vector<Commander> commanders_;
    int roundNum_;
};

#endif
