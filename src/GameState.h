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

#include <array>
#include <vector>

class Action;
class HexGrid;

class GameState
{
public:
    GameState(const HexGrid &bfGrid);

    void nextTurn();
    int getRound() const;

    // Units are owned by their GameState instance.
    void addUnit(Unit u);
    Unit & getUnit(int id);
    const Unit & getUnit(int id) const;
    Unit & getActiveUnit();

    Unit * getUnitAt(int aIndex);
    const Unit * getUnitAt(int aIndex) const;

    void moveUnit(Unit &u, int aDest);
    void assignDamage(Unit &u, int damage);

    // Return the list of adjacent enemy units, possibly using a different hex
    // from the one the unit is standing in.
    std::vector<Unit *> getAdjEnemies(const Unit &unit);
    std::vector<Unit *> getAdjEnemies(const Unit &unit, int aIndex);
    // NOTE: Normally you want non-const Units so you can attack them.  It's
    // not worth the effort to make const versions of these functions.

    std::vector<Unit *> getAllEnemies(const Unit &unit);

    // Score the current battle state for each side.  Normalize each unit by
    // comparing size to growth rate.
    std::array<int, 2> getScore() const;

    // Leaders of each army.
    void setCommander(Commander c, int team);
    Commander & getCommander(int team);
    const Commander & getCommander(int team) const;

    bool isRangedAttackAllowed(const Unit &attacker) const;
    bool isRetaliationAllowed(const Action &action) const;

    // Get the shortest path between two hexes that is clear of units.
    std::vector<int> getPath(int aSrc, int aTgt) const;

    // TODO: take entity ids instead of Unit refs
    Action makeMove(Unit &unit, int aTgt) const;
    Action makeAttack(Unit &attacker, Unit &defender, int aMoveTgt) const;
    Action makeSkip(Unit &unit) const;
    Action makeRetaliation(const Action &action) const;

    int computeDamage(const Action &action) const;
    int getSimulatedDamage(const Action &action) const;
    void execute(const Action &action);

    // Generate the set of all possible actions for the active unit.
    std::vector<Action> getPossibleActions();

private:
    void nextRound();

    // Rebuild the mapping of unit positions.  Call this whenever 'units_' is
    // invalidated.
    void remapUnitPos();

    // Compute a weighting factor applied to attack damage influenced by the
    // commanders of both teams.
    double getDamageMultiplier(const Action &action) const;

    // Get list of neighboring hexes that are free of units.
    std::vector<int> getOpenNeighbors(int aIndex) const;

    const HexGrid &grid_;
    std::vector<Unit> units_;  // indexed by entity id
    std::vector<int> turnOrder_;
    int curTurn_;
    std::vector<int> unitAtPos_;
    std::vector<Commander> commanders_;
    int roundNum_;
};

#endif
