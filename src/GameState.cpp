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
#include "UnitType.h"

#include <algorithm>
#include <cassert>

GameState::GameState(int bfSize)
    : units_{},
    activeUnit_{std::end(units_)},
    unitIdxAtPos_(bfSize, -1),
    commanders_(2),
    roundNum_{0}
{
}

void GameState::nextTurn()
{
    if (roundNum_ == 0) {
        nextRound();
        activeUnit_ = std::begin(units_);
        return;
    }

    activeUnit_ = find_if(std::next(activeUnit_), std::end(units_),
                          std::mem_fn(&Unit::isAlive));
    if (activeUnit_ != std::end(units_)) {
        return;
    }

    nextRound();
    activeUnit_ = find_if(std::begin(units_), std::end(units_),
                          std::mem_fn(&Unit::isAlive));
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
    if (activeUnit_ == std::end(units_)) return nullptr;
    return &(*activeUnit_);
}

Unit * GameState::getUnitAt(int aIndex)
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
