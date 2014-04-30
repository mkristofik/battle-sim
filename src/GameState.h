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
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "Commander.h"
#include "Unit.h"
#include "sdl_helper.h"

#include <array>
#include <functional>
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
    int getActiveTeam() const;

    // Units are owned by their GameState instance.
    void addUnit(Unit u);
    Unit & getUnit(int id);
    const Unit & getUnit(int id) const;
    Unit & getActiveUnit();
    const Unit & getActiveUnit() const;
    const Unit & getUnitAt(int aIndex) const;

    // Return true if there's no unit in the given hex.
    bool isHexOpen(int aIndex) const;

    // Return true if the hex is open and a flying unit can reach it.
    bool isHexFlyable(const Unit &unit, int aIndex) const;

    void moveUnit(int id, int aDest);
    int assignDamage(int id, int damage);

    // Return the list of adjacent enemy units, possibly using a different hex
    // from the one the unit is standing in.
    std::vector<int> getAdjEnemies(int id) const;
    std::vector<int> getAdjEnemies(int id, int aIndex) const;

    std::vector<int> getAllEnemies(int id) const;

    // Score the current battle state for each side.  Normalize each unit by
    // comparing size to growth rate.
    std::array<int, 2> getScore() const;
    bool isGameOver() const;
    bool isActiveTeamWinning() const;

    // Leaders of each army.
    void setCommander(Commander c, int team);
    const Commander & getCommander(int team) const;

    bool canUseMeleeAttack(int attId) const;
    bool isMeleeAttackAllowed(int attId, int defId) const;
    bool canUseRangedAttack(int attId) const;
    bool isRangedAttackAllowed(int attId, int defId) const;
    bool canUseSpell(int attId) const;
    bool isSpellAllowed(int attId, int defId) const;
    bool isRetaliationAllowed(const Action &action) const;
    bool isFirstStrikeAllowed(const Action &action) const;
    bool isDoubleStrikeAllowed(const Action &action) const;
    bool isTrampleAllowed(const Action &action) const;
    bool isBindAllowed(const Action &action) const;

    // Get the shortest path between two hexes that is clear of units.
    std::vector<int> getPath(int aSrc, int aTgt) const;
    std::vector<int> getPath(const Unit &unit, int aTgt) const;

    Action makeMove(int id, int aTgt) const;
    Action makeAttack(int attId, int defId, int aMoveTgt) const;
    Action makeSkip(int id) const;
    Action makeRetaliation(const Action &action) const;
    Action makeRegeneration(int id) const;
    Action makeBind(int attId, int defId) const;

    int computeDamage(const Action &action) const;
    void execute(const Action &action);

    // Generate the set of all possible actions for the active unit.
    std::vector<Action> getPossibleActions() const;

    void printAction(std::ostream &ostr, const Action &action) const;

    // Set the action execution function.  This callback allows us to run
    // animations separate from the game state context.
    void setExecFunc(std::function<void (Action)> f);

    // Use simulated damage for all actions.  Execute actions internally only.
    // Do not use the callback function.
    void setSimMode();

    // Encapsulate running an action and any retaliations or other special
    // abilities.  Runs the action callback for the given action and every
    // other action generated.
    void runActionSeq(Action action);

    // Track mana use when casting spells.  One mana is produced per turn per
    // team.  Turn 1 has 1 mana available for each team, 2 mana on turn 2, 3 on
    // turn 3, etc.
    int getMana(int team) const;
    int getManaLeft(int team) const;

private:
    void nextRound();

    // Rebuild the mapping of unit positions.  Call this whenever 'units_' is
    // invalidated.
    void remapUnitPos();

    // When units tie for initiative, make sure we alternate teams.
    void alternateTeamInitiative();
    void alternateTeams(int turnOrderBegin, int turnOrderEnd);

    // Compute a weighting factor applied to attack damage influenced by the
    // commanders of both teams.
    double getDamageMultiplier(const Action &action) const;

    // Get list of neighboring hexes that are free of units.
    std::vector<int> getOpenNeighbors(int aIndex) const;

    // Get list of all hexes the given unit can reach that are free of units.
    std::vector<int> getReachableHexes(const Unit &unit) const;

    // Use simulated damage when executing actions.
    void simulate(Action action);

    void actionCallback(Action action);
    void onStartTurn();

    const HexGrid &grid_;
    std::vector<Unit> units_;
    std::vector<int> turnOrder_;
    int curTurn_;
    std::vector<int> unitAtPos_;
    int roundNum_;
    std::function<void (Action)> execFunc_;
    bool simMode_;
    int drawTimer_;  // stalemate if no units killed several rounds in a row
    std::vector<int> mana_;
    std::vector<int> manaLeft_;
    static std::vector<Commander> commanders_;
};

#endif
