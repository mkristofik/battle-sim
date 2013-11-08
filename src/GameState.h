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

class Action;
class HexGrid;

class GameState
{
public:
    GameState(const HexGrid &bfGrid);

    void nextTurn();
    int getRound() const;

    void addUnit(Unit u);

    // Unit handles might change over time.  Best to not hold on to them for
    // longer than the current round.
    Unit * getActiveUnit();
    Unit * getUnitAt(int aIndex);
    const Unit * getUnitAt(int aIndex) const;

    void moveUnit(Unit &u, int aDest);

    // Return the list of adjacent enemy units, possibly using a different hex
    // from the one the unit is standing in.
    std::vector<Unit *> getAdjEnemies(const Unit &unit);
    std::vector<Unit *> getAdjEnemies(const Unit &unit, int aIndex);
    // NOTE: Normally you want non-const Units so you can attack them.  It's
    // not worth the effort to make const versions of these functions.

    // Score the current battle state for each side.  Normalize each unit by
    // comparing size to growth rate.
    std::pair<int, int> getScore() const;

    // Leaders of each army.
    void setCommander(Commander c, int team);
    Commander & getCommander(int team);
    const Commander & getCommander(int team) const;

    // Set the damage field of an action.
    void computeDamage(Action &action) const;

    // Return true if the given unit has a ranged attack and there are no
    // enemies adjacent to it.
    bool isRangedAttackAllowed(const Unit &attacker) const;

    // Get the shortest path between two hexes that is clear of units.
    std::vector<int> getPath(int aSrc, int aTgt) const;

    Action makeMove(Unit &unit, int aTgt) const;
    Action makeAttack(Unit &attacker, Unit &defender, int aMoveTgt) const;
    Action makeSkip(Unit &unit) const;

private:
    void nextRound();

    // Rebuild the mapping of unit positions.  Call this whenever 'units_' is
    // invalidated.
    void remapUnitPos();

    const HexGrid &grid_;
    std::vector<Unit> units_;
    int activeUnit_;
    std::vector<int> unitIdxAtPos_;
    std::vector<Commander> commanders_;
    int roundNum_;
};

#endif
