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

namespace
{
    Unit nullUnit;
}

GameState::GameState(const HexGrid &bfGrid)
    : grid_(bfGrid),
    units_{},
    turnOrder_{},
    curTurn_{-1},
    unitAtPos_(grid_.size(), -1),
    commanders_(2),
    roundNum_{0}
{
}

void GameState::nextTurn()
{
    if (roundNum_ == 0) {
        nextRound();
        return;
    }

    if (turnOrder_.empty()) return;

    ++curTurn_;
    int endOfRound = turnOrder_.size();
    while (curTurn_ < endOfRound && !units_[turnOrder_[curTurn_]].isAlive()) {
        ++curTurn_;
    }

    if (curTurn_ == endOfRound) {
        nextRound();
    }
}

int GameState::getRound() const
{
    return roundNum_;
}

void GameState::addUnit(Unit u)
{
    assert(unitAtPos_[u.aHex] == -1);

    int id = u.entityId;
    if (id >= static_cast<int>(units_.size())) {
        units_.resize(id + 1);
    }
    unitAtPos_[u.aHex] = id;
    units_[id] = std::move(u);
}

Unit & GameState::getUnit(int id)
{
    return const_cast<Unit &>(static_cast<const GameState *>(this)->getUnit(id));
 }

const Unit & GameState::getUnit(int id) const
{
    if (id < 0 || id >= static_cast<int>(units_.size())) return nullUnit;
    if (!units_[id].isValid()) return nullUnit;

    return units_[id];
}

Unit & GameState::getActiveUnit()
{
    if (curTurn_ == -1) return nullUnit;
    return units_[turnOrder_[curTurn_]];
}

Unit * GameState::getUnitAt(int aIndex)
{
    return const_cast<Unit *>(static_cast<const GameState *>(this)->getUnitAt(aIndex));
}

const Unit * GameState::getUnitAt(int aIndex) const
{
    assert(aIndex >= 0 && aIndex < static_cast<int>(unitAtPos_.size()));

    auto id = unitAtPos_[aIndex];
    if (id == -1) return nullptr;

    auto &unit = units_[id];
    assert(unit.aHex == aIndex);
    if (unit.isAlive()) {
        return &unit;
    }

    return nullptr;
}

void GameState::moveUnit(Unit &u, int aDest)
{
    assert(unitAtPos_[aDest] == -1);

    unitAtPos_[u.aHex] = -1;
    unitAtPos_[aDest] = u.entityId;
    u.aHex = aDest;
}

void GameState::assignDamage(Unit &u, int damage)
{
    u.takeDamage(damage);
    if (!u.isAlive()) {
        unitAtPos_[u.aHex] = -1;
    }
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

std::vector<Unit *> GameState::getAllEnemies(const Unit &unit)
{
    std::vector<Unit *> enemies;

    for (auto &u : units_) {
        if (u.isAlive() && unit.isEnemy(u)) {
            enemies.push_back(&u);
        }
    }

    return enemies;
}

std::array<int, 2> GameState::getScore() const
{
    std::array<int, 2> score;
    score.fill(0);

    for (auto &u : units_) {
        if (u.isValid()) {
            assert(u.team >= 0 && u.team < static_cast<int>(score.size()));
            score[u.team] += u.num * 100 / u.type->growth;
        }
    }
    return score;
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

bool GameState::isRetaliationAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    return action.type == ActionType::ATTACK &&
           att.isAlive() &&
           def.isAlive() &&
           !def.retaliated;
}

std::vector<int> GameState::getPath(int aSrc, int aTgt) const
{
    Pathfinder pf;
    pf.setNeighbors([&] (int aIndex) {return getOpenNeighbors(aIndex);});
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
    action.attacker = unit.entityId;
    action.path = path;
    return action;
}

Action GameState::makeAttack(Unit &attacker, Unit &defender, int aMoveTgt) const
{
    Action action;
    action.attacker = attacker.entityId;
    action.defender = defender.entityId;
    action.aTgt = defender.aHex;

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
    action.attacker = unit.entityId;
    return action;
}

Action GameState::makeRetaliation(const Action &action) const
{
    Action retaliation;
    retaliation.attacker = action.defender;
    retaliation.defender = action.attacker;
    retaliation.type = ActionType::RETALIATE;
    retaliation.aTgt = getUnit(retaliation.defender).aHex;
    return retaliation;
}

int GameState::computeDamage(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    if (!att.isAlive() || !def.isAlive()) return 0;
    return att.num * att.randomDamage(action.type) *
        getDamageMultiplier(action);
}

int GameState::getSimulatedDamage(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    if (!att.isAlive() || !def.isAlive()) return 0;
    return att.num * att.avgDamage(action.type) * getDamageMultiplier(action);
}

void GameState::execute(const Action &action)
{
    auto &att = getUnit(action.attacker);
    auto &def = getUnit(action.defender);
    if (!att.isAlive() || action.type == ActionType::NONE) return;

    if (action.path.size() > 1) {
        moveUnit(att, action.path.back());
    }

    if (action.type == ActionType::ATTACK ||
        action.type == ActionType::RANGED ||
        action.type == ActionType::RETALIATE)
    {
        assert(def.isAlive());
        assignDamage(def, action.damage);
    }

    if (action.type == ActionType::RETALIATE) {
        att.retaliated = true;
    }
}

std::vector<Action> GameState::getPossibleActions()
{
    auto &unit = getActiveUnit();
    if (!unit.isAlive()) return {};

    Pathfinder pf;
    pf.setNeighbors([&] (int aIndex) {return getOpenNeighbors(aIndex);});
    auto reachableHexes = pf.getReachableNodes(unit.aHex, unit.type->moves);

    std::vector<Action> actions;

    if (isRangedAttackAllowed(unit)) {
        for (auto e : getAllEnemies(unit)) {
            actions.emplace_back(makeAttack(unit, *e, -1));
        }
    }
    else {
        for (auto aHex : reachableHexes) {
            for (auto e : getAdjEnemies(unit, aHex)) {
                actions.emplace_back(makeAttack(unit, *e, aHex));
            }
        }
    }

    for (auto aHex : reachableHexes) {
        if (aHex != unit.aHex) {
            actions.emplace_back(makeMove(unit, aHex));
        }
    }

    actions.emplace_back(makeSkip(unit));
    return actions;
}

void GameState::nextRound()
{
    turnOrder_.clear();
    for (const auto &u : units_) {
        if (u.isAlive()) {
            turnOrder_.push_back(u.entityId);
        }
    }

    auto sortByInitiative = [this] (int lhs, int rhs) {
        return units_[lhs].type->initiative > units_[rhs].type->initiative;
    };
    stable_sort(std::begin(turnOrder_), std::end(turnOrder_), sortByInitiative);
    curTurn_ = (turnOrder_.empty() ? -1 : 0);

    for_each(std::begin(turnOrder_), std::end(turnOrder_),
             [this] (int id) {units_[id].retaliated = false;});

    remapUnitPos();
    ++roundNum_;
}

void GameState::remapUnitPos()
{
    fill(std::begin(unitAtPos_), std::end(unitAtPos_), -1);

    for (const auto &unit : units_) {
        if (unit.isAlive()) {
            unitAtPos_[unit.aHex] = unit.entityId;
        }
    }
}

double GameState::getDamageMultiplier(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    if (!att.isAlive() || !def.isAlive()) return 0;

    double attackBonus = 1.0;
    int attackDiff = getCommander(att.team).attack -
        getCommander(def.team).defense;
    if (attackDiff > 0) {
        attackBonus += attackDiff * 0.1;
    }
    else {
        attackBonus += attackDiff * 0.05;
    }
    attackBonus = bound(attackBonus, 0.3, 3.0);

    return attackBonus;
}

std::vector<int> GameState::getOpenNeighbors(int aIndex) const
{
    std::vector<int> nbrs;
    for (auto n : grid_.aryNeighbors(aIndex)) {
        if (!getUnitAt(n)) {
            nbrs.push_back(n);
        }
    }
    return nbrs;
}
