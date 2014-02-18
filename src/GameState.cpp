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
#include "GameState.h"

#include "Action.h"
#include "HexGrid.h"
#include "Pathfinder.h"
#include "Traits.h"
#include "UnitType.h"
#include "algo.h"

#include <algorithm>
#include <cassert>
#include <ostream>

namespace
{
    Unit nullUnit;

    void nullExecFunc(Action)
    {
        assert(false);
    }
}

std::vector<Commander> GameState::commanders_;

GameState::GameState(const HexGrid &bfGrid)
    : grid_(bfGrid),
    units_{},
    turnOrder_{},
    curTurn_{-1},
    unitAtPos_(grid_.size(), -1),
    roundNum_{0},
    execFunc_{nullExecFunc},
    simMode_{false}
{
    commanders_.resize(2);
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

    onStartTurn();
}

int GameState::getRound() const
{
    return roundNum_;
}

int GameState::getActiveTeam() const
{
    const Unit &unit = getActiveUnit();
    assert(unit.isValid());
    return unit.team;
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
    if (!units_[id].isValid()) {
        std::cerr << "WARNING: returning null unit, entity id " << id << " is not valid." << std::endl;
        return nullUnit;
    }

    return units_[id];
}

const Unit & GameState::getActiveUnit() const
{
    if (curTurn_ == -1) return nullUnit;
    return units_[turnOrder_[curTurn_]];
}

const Unit & GameState::getUnitAt(int aIndex) const
{
    assert(aIndex >= 0 && aIndex < static_cast<int>(unitAtPos_.size()));

    auto id = unitAtPos_[aIndex];
    if (id == -1) return nullUnit;

    auto &unit = units_[id];
    assert(unit.aHex == aIndex);
    if (unit.isAlive()) {
        return unit;
    }

    return nullUnit;
}

bool GameState::isHexOpen(int aIndex) const
{
    if (grid_.offGrid(aIndex)) return false;
    return !getUnitAt(aIndex).isAlive();
}

bool GameState::isHexFlyable(const Unit &unit, int aIndex) const
{
    if (!unit.hasTrait(Trait::FLYING)) return false;
    if (unit.aHex == aIndex) return true;

    if (isHexOpen(aIndex) &&
        grid_.aryDist(unit.aHex, aIndex) <= unit.type->moves)
    {
        return true;
    }

    return false;
}

void GameState::moveUnit(int id, int aDest)
{
    assert(unitAtPos_[aDest] == -1);

    auto &unit = getUnit(id);
    assert(unit.isValid());

    unitAtPos_[unit.aHex] = -1;
    unitAtPos_[aDest] = unit.entityId;
    unit.aHex = aDest;
}

int GameState::assignDamage(int id, int damage)
{
    auto &unit = getUnit(id);
    assert(unit.isValid());

    int numKilled = unit.takeDamage(damage);
    if (!unit.isAlive()) {
        unitAtPos_[unit.aHex] = -1;
    }

    return numKilled;
}

std::vector<int> GameState::getAdjEnemies(int id) const
{
    const auto &unit = getUnit(id);
    return getAdjEnemies(id, unit.aHex);
}

std::vector<int> GameState::getAdjEnemies(int id, int aIndex) const
{
    const auto &unit = getUnit(id);
    if (!unit.isAlive()) return {};

    std::vector<int> enemies;

    for (auto n : grid_.aryNeighbors(aIndex)) {
        const auto &adjUnit = getUnitAt(n);
        if (adjUnit.isAlive() && unit.isEnemy(adjUnit)) {
            enemies.push_back(adjUnit.entityId);
        }
    }

    return enemies;
}

std::vector<int> GameState::getAllEnemies(int id) const
{
    const auto &unit = getUnit(id);
    if (!unit.isAlive()) return {};

    std::vector<int> enemies;

    for (const auto &u : units_) {
        if (u.isAlive() && unit.isEnemy(u)) {
            enemies.push_back(u.entityId);
        }
    }

    return enemies;
}

std::array<int, 2> GameState::getScore() const
{
    std::array<int, 2> score;
    score.fill(0);

    for (auto &u : units_) {
        if (!u.isAlive()) continue;

        assert(u.team >= 0 && u.team < static_cast<int>(score.size()));
        int unitScore = (u.num - 1) * 100 / u.type->growth;
        unitScore += (100.0 * u.hpLeft / u.type->hp) / u.type->growth;
        score[u.team] += std::max(unitScore, 1);
    }
    return score;
}

void GameState::setCommander(Commander c, int team)
{
    assert(team == 0 || team == 1);
    commanders_[team] = std::move(c);
}

const Commander & GameState::getCommander(int team) const
{
    assert(team == 0 || team == 1);
    return commanders_[team];
}

bool GameState::isMeleeAttackAllowed(int id) const
{
    const auto &attacker = getUnit(id);
    return attacker.isAlive() && !attacker.type->hasRangedAttack;
}

bool GameState::isRangedAttackAllowed(int id) const
{
    const auto &attacker = getUnit(id);
    if (!attacker.isAlive() || !attacker.type->hasRangedAttack) return false;

    return getAdjEnemies(id).empty();
}

bool GameState::isRetaliationAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    return action.type == ActionType::ATTACK &&
           att.isAlive() &&
           def.isAlive() &&
           isMeleeAttackAllowed(action.defender) &&
           !def.retaliated &&
           (!att.hasTrait(Trait::FLYING) || def.hasTrait(Trait::FLYING));
}

bool GameState::isFirstStrikeAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    return isRetaliationAllowed(action) &&
           !att.hasTrait(Trait::FIRST_STRIKE) &&
           def.hasTrait(Trait::FIRST_STRIKE);
}

bool GameState::isDoubleStrikeAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);

    if (!att.isAlive() || !def.isAlive()) return false;
    if (!att.hasTrait(Trait::DOUBLE_STRIKE)) return false;

    if (action.type == ActionType::RANGED) {
        return true;
    }
    if (action.type == ActionType::ATTACK &&
        isMeleeAttackAllowed(action.attacker))
    {
        return true;
    }

    return false;
}

bool GameState::isTrampleAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);

    if (!att.hasTrait(Trait::TRAMPLE)) return false;
    if (action.type != ActionType::ATTACK) return false;

    return att.isAlive() && !def.isAlive();
}

std::vector<int> GameState::getPath(int aSrc, int aTgt) const
{
    if (grid_.offGrid(aSrc) || grid_.offGrid(aTgt)) return {};

    Pathfinder pf;
    pf.setNeighbors([&] (int aIndex) {return getOpenNeighbors(aIndex);});
    pf.setGoal(aTgt);
    return pf.getPathFrom(aSrc);
}

std::vector<int> GameState::getPath(const Unit &unit, int aTgt) const
{
    if (grid_.offGrid(aTgt)) return {};

    if (isHexFlyable(unit, aTgt)) {
        std::vector<int> path;
        path.push_back(unit.aHex);
        if (unit.aHex != aTgt) {
            path.push_back(aTgt);
        }
        return path;
    }

    return getPath(unit.aHex, aTgt);
}

Action GameState::makeMove(int id, int aTgt) const
{
    const auto &unit = getUnit(id);
    if (!unit.isAlive()) return {};

    auto path = getPath(unit, aTgt);
    if (path.size() <= 1 || path.size() > unit.getMaxPathSize()) {
        return {};
    }

    Action action;
    action.type = ActionType::MOVE;
    action.attacker = id;
    action.path = path;
    return action;
}

Action GameState::makeAttack(int attId, int defId, int aMoveTgt) const
{
    const auto &attacker = getUnit(attId);
    const auto &defender = getUnit(defId);
    if (!attacker.isAlive() || !defender.isAlive()) return {};

    Action action;
    action.attacker = attId;
    action.defender = defId;
    action.aTgt = defender.aHex;

    if (isRangedAttackAllowed(attId)) {
        action.type = ActionType::RANGED;
        return action;
    }
    if (!isMeleeAttackAllowed(attId)) {
        return {};
    }

    auto path = getPath(attacker, aMoveTgt);
    if (path.empty() || path.size() > attacker.getMaxPathSize()) {
        return {};
    }
    action.type = ActionType::ATTACK;
    action.path = path;
    return action;
}

Action GameState::makeSkip(int id) const
{
    Action action;
    action.type = ActionType::NONE;
    action.attacker = id;
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

Action GameState::makeRegeneration(int id) const
{
    Action regen;
    regen.attacker = id;
    regen.type = ActionType::REGENERATE;
    return regen;
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
        moveUnit(action.attacker, action.path.back());
    }

    if (action.type == ActionType::ATTACK ||
        action.type == ActionType::RANGED ||
        action.type == ActionType::RETALIATE)
    {
        assert(def.isAlive());
        auto numKilled = assignDamage(action.defender, action.damage);

        if (att.hasTrait(Trait::LIFE_DRAIN)) {
            att.hpLeft = std::min(att.hpLeft + action.damage, att.type->hp);
        }
        if (att.hasTrait(Trait::ZOMBIFY)) {
            att.num += numKilled;
        }
    }

    if (action.type == ActionType::RETALIATE) {
        att.retaliated = true;
    }

    if (action.type == ActionType::REGENERATE) {
        att.hpLeft = att.type->hp;
    }
}

std::vector<Action> GameState::getPossibleActions() const
{
    const auto &unit = getActiveUnit();
    assert(unit.isAlive());

    auto reachableHexes = getReachableHexes(unit);
    std::vector<Action> actions;

    if (isRangedAttackAllowed(unit.entityId)) {
        for (auto e : getAllEnemies(unit.entityId)) {
            actions.emplace_back(makeAttack(unit.entityId, e, -1));
        }
    }
    else if (isMeleeAttackAllowed(unit.entityId)) {
        for (auto aHex : reachableHexes) {
            for (auto e : getAdjEnemies(unit.entityId, aHex)) {
                actions.emplace_back(makeAttack(unit.entityId, e, aHex));
            }
        }
    }

    actions.emplace_back(makeSkip(unit.entityId));

    for (auto aHex : reachableHexes) {
        if (aHex != unit.aHex) {
            actions.emplace_back(makeMove(unit.entityId, aHex));
        }
    }

    return actions;
}

void GameState::printAction(std::ostream &ostr, const Action &action) const
{
    switch (action.type) {
        case ActionType::NONE:
            ostr << "Skip turn";
            break;
        case ActionType::MOVE:
            assert(!action.path.empty());
            ostr << "Move to " << action.path.back();
            break;
        case ActionType::ATTACK:
        {
            assert(!action.path.empty());

            const auto &att = getUnit(action.attacker);
            const auto &def = getUnit(action.defender);
            assert(att.isValid());
            assert(def.isValid());

            auto moveTgt = action.path.back();
            if (moveTgt != att.aHex) {
                ostr << "Move to " << moveTgt << ' ';
            }
            ostr << "Melee attack " << def.getName();
            break;
        }
        case ActionType::RANGED:
        {
            const auto &def = getUnit(action.defender);
            assert(def.isValid());
            ostr << "Ranged attack " << def.getName();
            break;
        }
        default:
            break;
    }
}

void GameState::setExecFunc(std::function<void (Action)> f)
{
    execFunc_ = std::move(f);
}

void GameState::setSimMode()
{
    simMode_ = true;
}

void GameState::runActionSeq(Action action)
{
    // The First Strike ability lets the defender get its retaliation in prior
    // to the actual attack.  We must therefore split the attack into two
    // separate actions, a move and (if the attacker is still alive) an attack.
    if (isFirstStrikeAllowed(action)) {
        if (action.path.size() > 1) {
            auto moveAction = action;
            moveAction.type = ActionType::MOVE;
            actionCallback(moveAction);
        }
        auto firstStrike = makeRetaliation(action);
        actionCallback(firstStrike);

        const auto &att = getUnit(action.attacker);
        if (att.isAlive()) {
            action = makeAttack(action.attacker, action.defender, att.aHex);
        }
        else {
            return;
        }
    }

    actionCallback(action);

    if (isRetaliationAllowed(action)) {
        auto retal = makeRetaliation(action);
        actionCallback(retal);
    }
    if (isDoubleStrikeAllowed(action)) {
        const auto &att = getUnit(action.attacker);
        auto secondStrike = makeAttack(att.entityId, action.defender, att.aHex);
        actionCallback(secondStrike);
    }
    if (isTrampleAllowed(action)) {
        const auto &def = getUnit(action.defender);
        auto trample = makeMove(action.attacker, def.aHex);
        actionCallback(trample);
    }
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
        if (isHexOpen(n)) {
            nbrs.push_back(n);
        }
    }
    return nbrs;
}

std::vector<int> GameState::getReachableHexes(const Unit &unit) const
{
    std::vector<int> reachable;

    // Flying units don't need a clear path.
    if (unit.hasTrait(Trait::FLYING)) {
        for (int i = 0; i < grid_.size(); ++i) {
            if (isHexFlyable(unit, i)) {
                reachable.push_back(i);
            }
        }
    }
    else {
        Pathfinder pf;
        pf.setNeighbors([&] (int aIndex) {return getOpenNeighbors(aIndex);});
        reachable = pf.getReachableNodes(unit.aHex, unit.type->moves);
    }

    return reachable;
}

void GameState::simulate(Action action)
{
    action.damage = getSimulatedDamage(action);
    execute(action);
}

void GameState::actionCallback(Action action)
{
    if (simMode_) {
        simulate(action);
    }
    else {
        execFunc_(action);
        // Note: this requires that execFunc_ refer to the same GameState as
        // 'this'.  If that becomes a problem, we can break this into
        // pre-callback, execute, post-callback.
    }
}

void GameState::onStartTurn()
{
    const auto &unit = getActiveUnit();
    assert(unit.isAlive());

    if (unit.hasTrait(Trait::REGENERATE) && unit.hpLeft < unit.type->hp) {
        auto regen = makeRegeneration(unit.entityId);
        runActionSeq(regen);
    }
}
