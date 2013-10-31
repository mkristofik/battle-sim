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
    : bfUnits_{},
    activeUnit_{std::end(bfUnits_)},
    unitIdxAtPos_(bfSize, -1),
    commanders_(2),
    roundNum_{0}
{
}

void GameState::nextTurn()
{
    if (roundNum_ == 0) {
        nextRound();
        activeUnit_ = std::begin(bfUnits_);
        return;
    }

    activeUnit_ = find_if(std::next(activeUnit_), std::end(bfUnits_),
                          std::mem_fn(&Unit::isAlive));
    if (activeUnit_ != std::end(bfUnits_)) {
        return;
    }

    nextRound();
    activeUnit_ = find_if(std::begin(bfUnits_), std::end(bfUnits_),
                          std::mem_fn(&Unit::isAlive));
}

int GameState::getRound() const
{
    return roundNum_;
}

void GameState::addUnit(Unit u)
{
    assert(unitIdxAtPos_[u.aHex] == -1);

    bfUnits_.emplace_back(std::move(u));
    // note: this invalidates the active unit
    unitIdxAtPos_[bfUnits_.back().aHex] = bfUnits_.size() - 1;
}

Unit * GameState::getActiveUnit()
{
    if (activeUnit_ == std::end(bfUnits_)) return nullptr;
    return &(*activeUnit_);
}

Unit * GameState::getUnitAt(int aIndex)
{
    assert(aIndex >= 0 && aIndex < static_cast<int>(unitIdxAtPos_.size()));

    auto unitIndex = unitIdxAtPos_[aIndex];
    if (unitIndex == -1) return nullptr;

    auto &unit = bfUnits_[unitIndex];
    assert(unit.aHex == aIndex);
    if (unit.isAlive()) {
        return &unit;
    }

    return nullptr;
}

void GameState::moveUnit(Unit &u, int aDest)
{
    assert(unitIdxAtPos_[aDest] == -1);

    int curHex = u.aHex;
    int unitIdx = unitIdxAtPos_[curHex];
    unitIdxAtPos_[curHex] = -1;
    unitIdxAtPos_[aDest] = unitIdx;
    u.aHex = aDest;
}

std::pair<int, int> GameState::getScore() const
{
    int score[] = {0, 0};
    for (auto &u : bfUnits_) {
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
    auto lastAlive = stable_partition(std::begin(bfUnits_), std::end(bfUnits_),
                                      std::mem_fn(&Unit::isAlive));
    stable_sort(std::begin(bfUnits_), lastAlive, sortByInitiative);
    for_each(std::begin(bfUnits_), lastAlive,
             [] (Unit &unit) {unit.retaliated = false;});
    remapUnitPos();
    ++roundNum_;
}

void GameState::remapUnitPos()
{
    fill(std::begin(unitIdxAtPos_), std::end(unitIdxAtPos_), -1);

    int size = bfUnits_.size();
    for (int i = 0; i < size; ++i) {
        const auto &unit = bfUnits_[i];
        if (unit.isAlive()) {
            unitIdxAtPos_[unit.aHex] = i;
        }
    }
}
