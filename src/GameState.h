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
#include <iosfwd>
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
    // TODO: only const refs should be public
    void addUnit(Unit u);
    Unit & getUnit(int id);
    const Unit & getUnit(int id) const;
    Unit & getActiveUnit();
    Unit & getUnitAt(int aIndex);
    const Unit & getUnitAt(int aIndex) const;

    void moveUnit(int id, int aDest);
    void assignDamage(int id, int damage);

    // Return the list of adjacent enemy units, possibly using a different hex
    // from the one the unit is standing in.
    std::vector<int> getAdjEnemies(int id) const;
    std::vector<int> getAdjEnemies(int id, int aIndex) const;

    std::vector<int> getAllEnemies(int id) const;

    // Score the current battle state for each side.  Normalize each unit by
    // comparing size to growth rate.
    std::array<int, 2> getScore() const;

    // Leaders of each army.
    void setCommander(Commander c, int team);
    Commander & getCommander(int team);
    const Commander & getCommander(int team) const;

    bool isRangedAttackAllowed(int id) const;
    bool isRetaliationAllowed(const Action &action) const;

    // Get the shortest path between two hexes that is clear of units.
    std::vector<int> getPath(int aSrc, int aTgt) const;

    Action makeMove(int id, int aTgt) const;
    Action makeAttack(int attId, int defId, int aMoveTgt) const;
    Action makeSkip(int id) const;
    Action makeRetaliation(const Action &action) const;

    int computeDamage(const Action &action) const;
    int getSimulatedDamage(const Action &action) const;
    void execute(const Action &action);

    // Generate the set of all possible actions for the active unit.
    std::vector<Action> getPossibleActions();

    void printAction(std::ostream &ostr, const Action &action) const;

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
