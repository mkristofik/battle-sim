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

GameState::GameState()
    : bfUnits_{},
    activeUnit_{std::end(bfUnits_)},
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
    bfUnits_.emplace_back(std::move(u));
    // note: this invalidates the active unit
}

Unit * GameState::getActiveUnit()
{
    if (activeUnit_ == std::end(bfUnits_)) return nullptr;
    return &(*activeUnit_);
}

Unit * GameState::getUnitAt(int aIndex)
{
    for (auto &u : bfUnits_) {
        if (u.aHex == aIndex && u.isAlive()) {
            return &u;
        }
    }

    return nullptr;
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
    ++roundNum_;
}
