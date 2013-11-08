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
#include "GameState.h"

#include "Action.h"
#include "HexGrid.h"
#include "Pathfinder.h"
#include "UnitType.h"
#include "algo.h"

#include <algorithm>
#include <cassert>

GameState::GameState(const HexGrid &bfGrid)
    : grid_(bfGrid),
    units_{},
    activeUnit_{1},
    unitIdxAtPos_(grid_.size(), -1),
    commanders_(2),
    roundNum_{0}
{
}

void GameState::nextTurn()
{
    if (roundNum_ == 0) {
        nextRound();
        activeUnit_ = 0;
        return;
    }

    std::vector<Unit>::iterator activeIter{&units_[activeUnit_ + 1]};
    activeIter = find_if(activeIter, std::end(units_),
                         std::mem_fn(&Unit::isAlive));

    if (activeIter == std::end(units_)) {
        nextRound();
        activeIter = find_if(std::begin(units_), std::end(units_),
                             std::mem_fn(&Unit::isAlive));
    }

    activeUnit_ = distance(std::begin(units_), activeIter);
}

int GameState::getRound() const
{
    return roundNum_;
}

void GameState::addUnit(Unit u)
{
    assert(unitIdxAtPos_[u.aHex] == -1);

    units_.emplace_back(std::move(u));
    // note: this invalidates the active unit
    unitIdxAtPos_[units_.back().aHex] = units_.size() - 1;
}

Unit * GameState::getActiveUnit()
{
    if (activeUnit_ == static_cast<int>(units_.size())) return nullptr;
    return &units_[activeUnit_];
}

Unit * GameState::getUnitAt(int aIndex)
{
    return const_cast<Unit *>(static_cast<const GameState *>(this)->getUnitAt(aIndex));
}

const Unit * GameState::getUnitAt(int aIndex) const
{
    assert(aIndex >= 0 && aIndex < static_cast<int>(unitIdxAtPos_.size()));

    auto unitIndex = unitIdxAtPos_[aIndex];
    if (unitIndex == -1) return nullptr;

    auto &unit = units_[unitIndex];
    assert(unit.aHex == aIndex);
    if (unit.isAlive()) {
        return &unit;
    }

    return nullptr;
}

void GameState::moveUnit(Unit &u, int aDest)
{
    // TODO: this assertion fails if a unit killed this round is at the
    // destination hex
    //assert(unitIdxAtPos_[aDest] == -1);

    int curHex = u.aHex;
    int unitIdx = unitIdxAtPos_[curHex];
    unitIdxAtPos_[curHex] = -1;
    unitIdxAtPos_[aDest] = unitIdx;
    u.aHex = aDest;
}

std::vector<Unit *> GameState::getAdjEnemies(const Unit &unit)
{
    return getAdjEnemies(unit, unit.aHex);
}

std::vector<Unit *> GameState::getAdjEnemies(const Unit &unit, int aIndex)
{
    std::vector<Unit *> enemies;

    for (auto n : grid_.aryNeighbors(aIndex)) {
        auto adjUnit = getUnitAt(n);
        if (adjUnit && unit.isEnemy(*adjUnit)) {
            enemies.push_back(adjUnit);
        }
    }

    return enemies;
}

std::pair<int, int> GameState::getScore() const
{
    int score[] = {0, 0};
    for (auto &u : units_) {
        assert(u.team == 0 || u.team == 1);
        score[u.team] += u.num * 100 / u.type->growth;
    }
    return {score[0], score[1]};
}

void GameState::setCommander(Commander c, int team)
{
    assert(team == 0 || team == 1);
    commanders_[team] = std::move(c);
}

Commander & GameState::getCommander(int team)
{
    assert(team == 0 || team == 1);
    return commanders_[team];
}

const Commander & GameState::getCommander(int team) const
{
    assert(team == 0 || team == 1);
    return commanders_[team];
}

void GameState::computeDamage(Action &action) const
{
    if (!action.attacker || !action.defender) return;

    double attackBonus = 1.0;
    int attackDiff = getCommander(action.attacker->team).attack -
        getCommander(action.defender->team).defense;
    if (attackDiff > 0) {
        attackBonus += attackDiff * 0.1;
    }
    else {
        attackBonus += attackDiff * 0.05;
    }
    attackBonus = bound(attackBonus, 0.3, 3.0);

    action.damage = action.attacker->num *
        action.attacker->randomDamage(action.type) * attackBonus;
}

bool GameState::isRangedAttackAllowed(const Unit &attacker) const
{
    if (!attacker.type->hasRangedAttack) return false;

    for (auto n : grid_.aryNeighbors(attacker.aHex)) {
        const auto adjUnit = getUnitAt(n);
        if (adjUnit && attacker.isEnemy(*adjUnit)) {
            return false;
        }
    }
    return true;
}

std::vector<int> GameState::getPath(int aSrc, int aTgt) const
{
    auto emptyHexes = [&] (int aIndex) {
        std::vector<int> nbrs;
        for (auto n : grid_.aryNeighbors(aIndex)) {
            if (!getUnitAt(n)) {
                nbrs.push_back(n);
            }
        }
        return nbrs;
    };

    Pathfinder pf;
    pf.setNeighbors(emptyHexes);
    pf.setGoal(aTgt);
    return pf.getPathFrom(aSrc);
}

Action GameState::makeMove(Unit &unit, int aTgt) const
{
    auto path = getPath(unit.aHex, aTgt);
    if (path.size() <= 1 || path.size() > unit.getMaxPathSize()) {
        return {};
    }

    Action action;
    action.type = ActionType::MOVE;
    action.attacker = &unit;
    action.path = path;
    return action;
}

Action GameState::makeAttack(Unit &attacker, Unit &defender, int aMoveTgt) const
{
    Action action;
    action.attacker = &attacker;
    action.defender = &defender;

    if (isRangedAttackAllowed(attacker)) {
        action.type = ActionType::RANGED;
        return action;
    }

    auto path = getPath(attacker.aHex, aMoveTgt);
    if (path.empty() || path.size() > attacker.getMaxPathSize()) {
        return {};
    }
    action.type = ActionType::ATTACK;
    action.path = path;
    return action;
}

Action GameState::makeSkip(Unit &unit) const
{
    Action action;
    action.type = ActionType::NONE;
    action.attacker = &unit;
    return action;
}

void GameState::nextRound()
{
    auto lastAlive = stable_partition(std::begin(units_), std::end(units_),
                                      std::mem_fn(&Unit::isAlive));
    stable_sort(std::begin(units_), lastAlive, sortByInitiative);
    for_each(std::begin(units_), lastAlive,
             [] (Unit &unit) {unit.retaliated = false;});
    remapUnitPos();
    ++roundNum_;
}

void GameState::remapUnitPos()
{
    fill(std::begin(unitIdxAtPos_), std::end(unitIdxAtPos_), -1);

    int size = units_.size();
    for (int i = 0; i < size; ++i) {
        const auto &unit = units_[i];
        if (unit.isAlive()) {
            unitIdxAtPos_[unit.aHex] = i;
        }
    }
}
