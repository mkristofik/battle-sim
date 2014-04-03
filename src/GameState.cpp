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
#include "Effects.h"
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
    const int ROUNDS_TO_DRAW = 4;

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
    simMode_{false},
    drawTimer_{ROUNDS_TO_DRAW}
{
    commanders_.resize(2);
}

void GameState::nextTurn()
{
    if (roundNum_ == 0) {
        nextRound();
        return;
    }

    if (isGameOver()) return;
    if (turnOrder_.empty()) return;

    ++curTurn_;
    int endOfRound = turnOrder_.size();
    while (curTurn_ < endOfRound && !getUnit(turnOrder_[curTurn_]).isAlive()) {
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
    unitAtPos_[u.aHex] = id;
    units_.emplace_back(std::move(u));
    stable_sort(std::begin(units_), std::end(units_),
        [] (const Unit &a, const Unit &b) { return a.entityId < b.entityId; });
}

Unit & GameState::getUnit(int id)
{
    return const_cast<Unit &>(static_cast<const GameState *>(this)->getUnit(id));
 }

const Unit & GameState::getUnit(int id) const
{
    auto iter = lower_bound(std::begin(units_), std::end(units_), id,
        [] (const Unit &a, int b) { return a.entityId < b; });
    if (iter == units_.end() || iter->entityId != id) return nullUnit;

    return *iter;
}

Unit & GameState::getActiveUnit()
{
    if (curTurn_ == -1) return nullUnit;
    return getUnit(turnOrder_[curTurn_]);
}

const Unit & GameState::getActiveUnit() const
{
    if (curTurn_ == -1) return nullUnit;
    return getUnit(turnOrder_[curTurn_]);
}

const Unit & GameState::getUnitAt(int aIndex) const
{
    assert(aIndex >= 0 && aIndex < static_cast<int>(unitAtPos_.size()));

    auto id = unitAtPos_[aIndex];
    if (id == -1) return nullUnit;

    const auto &unit = getUnit(id);
    assert(unit.aHex == aIndex);
    if (!unit.isAlive()) return nullUnit;

    return unit;
}

bool GameState::isHexOpen(int aIndex) const
{
    if (grid_.offGrid(aIndex)) return false;
    return !getUnitAt(aIndex).isAlive();
}

bool GameState::isHexFlyable(const Unit &unit, int aIndex) const
{
    if (!unit.canFly()) return false;
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
    if (numKilled > 0) {
        drawTimer_ = ROUNDS_TO_DRAW;
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

    if (drawTimer_ <= 0) return score;

    for (const auto &u : units_) {
        if (!u.isAlive()) continue;

        assert(u.team >= 0 && u.team < static_cast<int>(score.size()));
        int unitScore = (u.num - 1) * 100 / u.type->growth;
        unitScore += ceil((100.0 * u.hpLeft / u.type->hp) / u.type->growth);
        score[u.team] += unitScore;
    }
    return score;
}

bool GameState::isGameOver() const
{
    auto score = getScore();
    return score[0] <= 0 || score[1] <= 0;
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

bool GameState::canUseMeleeAttack(int attId) const
{
    const auto &att = getUnit(attId);
    if (!att.isAlive()) return false;

    if (att.hasTrait(Trait::RANGED) ||
        att.hasTrait(Trait::SPELLCASTER))
    {
        return false;
    }

    return true;
}

bool GameState::isMeleeAttackAllowed(int attId, int defId) const
{
    if (!canUseMeleeAttack(attId)) return false;

    const auto &att = getUnit(attId);
    const auto &def = getUnit(defId);
    return def.isAlive() && att.isEnemy(def);
}

bool GameState::canUseRangedAttack(int attId) const
{
    const auto &att = getUnit(attId);
    if (!att.isAlive()) return false;
    if (att.hasTrait(Trait::SPELLCASTER)) return false;

    return getAdjEnemies(attId).empty() && att.hasTrait(Trait::RANGED);
}

bool GameState::isRangedAttackAllowed(int attId, int defId) const
{
    if (!canUseRangedAttack(attId)) return false;

    const auto &att = getUnit(attId);
    const auto &def = getUnit(defId);
    return def.isAlive() && att.isEnemy(def);
}

bool GameState::canUseSpell(int attId) const
{
    const auto &att = getUnit(attId);
    if (!att.isAlive()) return false;

    return att.type->spell != nullptr && att.hasTrait(Trait::SPELLCASTER);
}

bool GameState::isSpellAllowed(int attId, int defId) const
{
    if (!canUseSpell(attId)) return false;

    const auto &att = getUnit(attId);
    const auto &def = getUnit(defId);
    if (!def.isAlive()) return false;

    const Spell *spell = att.type->spell;
    return (spell->target == SpellTarget::ENEMY && att.isEnemy(def)) ||
        (spell->target == SpellTarget::FRIEND && !att.isEnemy(def));
}

bool GameState::isRetaliationAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);

    if (action.type != ActionType::ATTACK) return false;
    if (!isMeleeAttackAllowed(action.defender, action.attacker)) return false;
    if (def.retaliated) return false;

    if (att.canFly() && !def.canFly()) {
        return false;
    }

    return true;
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
        isMeleeAttackAllowed(action.attacker, action.defender))
    {
        return true;
    }

    return false;
}

bool GameState::isTrampleAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);

    if (action.type != ActionType::ATTACK) return false;
    if (!att.hasTrait(Trait::TRAMPLE)) return false;
    if (att.getMaxPathSize() < 2) return false;

    return att.isAlive() && !def.isAlive();
}

bool GameState::isBindAllowed(const Action &action) const
{
    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);

    if (action.type != ActionType::ATTACK &&
        action.type != ActionType::RETALIATE)
    {
        return false;
    }
    if (def.hasEffect(EffectType::BOUND)) return false;
    if (!att.hasTrait(Trait::BINDING)) return false;

    return att.isAlive() && def.isAlive();
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

    bool attMoved = (aMoveTgt != attacker.aHex);

    if (!attMoved && isRangedAttackAllowed(attId, defId)) {
        action.type = ActionType::RANGED;
        return action;
    }
    if (!attMoved && isSpellAllowed(attId, defId)) {
        action.type = ActionType::EFFECT;
        action.damage = attacker.num * attacker.type->spell->damage;
        action.effect = Effect(*this, action, attacker.type->spell->effect);
        return action;
    }

    bool isAdjacent = contains(grid_.aryNeighbors(aMoveTgt), defender.aHex);
    if (!isAdjacent || !isMeleeAttackAllowed(attId, defId)) {
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
    const auto &target = getUnit(id);

    Action regen;
    regen.defender = target.entityId;
    regen.damage = target.hpLeft - target.type->hp;
    regen.type = ActionType::EFFECT;
    regen.effect = Effect(*this, regen, EffectType::HEAL);
    return regen;
}

Action GameState::makeBind(int attId, int defId) const
{
    Action binder;
    binder.attacker = attId;
    binder.defender = defId;
    binder.type = ActionType::EFFECT;
    binder.effect = Effect(*this, binder, EffectType::BOUND);
    return binder;
}

int GameState::computeDamage(const Action &action) const
{
    if (action.type == ActionType::NONE) return 0;

    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    int damage = action.damage;

    // Spells and traits have already computed damage.
    if (action.type != ActionType::EFFECT) {
        damage = att.num * att.randomDamage(action.type) *
            getDamageMultiplier(action);
    }

    // Healing effects need to not put a unit beyond max HP.
    if (damage < 0 && def.isAlive()) {
        int maxHealing = def.hpLeft - def.type->hp;
        damage = std::max(damage, maxHealing);
    }

    return damage;
}

int GameState::getSimulatedDamage(const Action &action) const
{
    // Spells and traits have already computed damage.
    if (action.type == ActionType::EFFECT) {
        return action.damage;
    }

    const auto &att = getUnit(action.attacker);
    const auto &def = getUnit(action.defender);
    if (!att.isAlive() || !def.isAlive()) return 0;
    return att.num * att.avgDamage(action.type) * getDamageMultiplier(action);
}

void GameState::execute(const Action &action)
{
    if (action.type == ActionType::NONE) return;

    auto &att = getUnit(action.attacker);
    auto &def = getUnit(action.defender);

    if (action.path.size() > 1) {
        assert(att.isAlive());
        moveUnit(action.attacker, action.path.back());
    }

    if (action.type == ActionType::ATTACK ||
        action.type == ActionType::RANGED ||
        action.type == ActionType::RETALIATE)
    {
        assert(att.isAlive());
        assert(def.isAlive());
        auto numKilled = assignDamage(action.defender, action.damage);

        if (att.hasTrait(Trait::LIFE_DRAIN)) {
            att.hpLeft = std::min(att.hpLeft + action.damage, att.type->hp);
        }
        if (att.hasTrait(Trait::ZOMBIFY)) {
            att.num += numKilled;
        }
    }
    else if (action.type == ActionType::EFFECT) {
        assert(def.isAlive());
        auto effect = action.effect;
        if (!effect.isDone()) {
            // Effects with duration stay with the defending unit.
            def.effect = effect;
        }

        assignDamage(action.defender, action.damage);
    }

    if (action.type == ActionType::RETALIATE &&
        !att.hasTrait(Trait::STEADFAST))
    {
        att.retaliated = true;
    }
}

std::vector<Action> GameState::getPossibleActions() const
{
    const auto &unit = getActiveUnit();
    assert(unit.isAlive());

    auto reachableHexes = getReachableHexes(unit);
    std::vector<Action> actions;

    if (canUseRangedAttack(unit.entityId)) {
        for (auto e : getAllEnemies(unit.entityId)) {
            actions.push_back(makeAttack(unit.entityId, e, unit.aHex));
        }
    }
    else if (canUseSpell(unit.entityId)) {
        for (const auto &target : units_) {
            if (!target.isAlive()) continue;
            auto possible = makeAttack(unit.entityId, target.entityId,
                                       unit.aHex);
            if (possible.type == ActionType::NONE) continue;
            actions.push_back(std::move(possible));
        }
    }
    else if (canUseMeleeAttack(unit.entityId)) {
        for (auto aHex : reachableHexes) {
            for (auto e : getAdjEnemies(unit.entityId, aHex)) {
                actions.push_back(makeAttack(unit.entityId, e, aHex));
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
        case ActionType::EFFECT:
        {
            const auto &def = getUnit(action.defender);
            assert(def.isValid());
            ostr << def.getName() << ' ' << action.effect.getText();
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

    if (isBindAllowed(action)) {
        auto binder = makeBind(action.attacker, action.defender);
        actionCallback(binder);
    }
    if (isRetaliationAllowed(action)) {
        auto retal = makeRetaliation(action);
        actionCallback(retal);

        // TODO: can we generalize this for any ability that happens on attack
        // and retaliate?
        if (isBindAllowed(retal)) {
            auto binder = makeBind(retal.attacker, retal.defender);
            actionCallback(binder);
        }
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
    curTurn_ = (turnOrder_.empty() ? -1 : 0);

    auto sortByInitiative = [this] (int lhs, int rhs) {
        return getUnit(lhs).type->initiative > getUnit(rhs).type->initiative;
    };
    stable_sort(std::begin(turnOrder_), std::end(turnOrder_), sortByInitiative);
    alternateTeamInitiative();

    for_each(std::begin(turnOrder_), std::end(turnOrder_),
             [this] (int id) {getUnit(id).retaliated = false;});

    remapUnitPos();
    --drawTimer_;
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

void GameState::alternateTeamInitiative()
{
    int size = turnOrder_.size();
    if (size < 2) return;

    // Break the turn order list into groups of equal initiative (assume it's
    // sorted) as though we called equal_range multiple times.  Make sure we
    // alternate teams within each group.
    int begin = 0;
    auto initiative = getUnit(turnOrder_[0]).type->initiative;
    for (int i = 0; i < size; ++i) {
        if (getUnit(turnOrder_[i]).type->initiative == initiative) continue;

        alternateTeams(begin, i);
        begin = i;
        initiative = getUnit(turnOrder_[begin]).type->initiative;
    }

    alternateTeams(begin, size);
}

void GameState::alternateTeams(int turnOrderBegin, int turnOrderEnd)
{
    bool team1sTurn = true;

    // Selection sort to alternate units from each team.
    for (int i = turnOrderBegin; i < turnOrderEnd; ++i) {
        for (int j = i; j < turnOrderEnd; ++j) {
            auto unitId = turnOrder_[j];
            if ((team1sTurn && getUnit(unitId).team == 0) ||
                (!team1sTurn && getUnit(unitId).team == 1))
            {
                std::rotate(&turnOrder_[i], &turnOrder_[j], &turnOrder_[j+1]);
                break;
            }
        }
        team1sTurn = !team1sTurn;
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
    if (unit.canFly()) {
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
    auto &unit = getActiveUnit();
    assert(unit.isAlive());

    if (unit.hasTrait(Trait::REGENERATE) && unit.hpLeft < unit.type->hp) {
        auto regen = makeRegeneration(unit.entityId);
        runActionSeq(regen);
    }

    if (unit.effect.type != EffectType::NONE) {
        unit.effect.apply(*this, unit);
        if (unit.effect.isDone()) {
            unit.effect = Effect();
        }
    }
}
