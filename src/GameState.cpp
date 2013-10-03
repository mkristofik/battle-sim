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

#include <algorithm>
#include <cassert>

std::unique_ptr<GameState> gs;

GameState::GameState()
    : entities_{},
    unitRef_{},
    bfUnits_{},
    activeUnit_{std::end(bfUnits_)},
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

Winner GameState::getWinner() const
{
    bool alive1 = any_of(std::begin(bfUnits_), std::end(bfUnits_),
        [] (const Unit &unit) { return unit.isAlive() && unit.team == 0; });
    bool alive2 = any_of(std::begin(bfUnits_), std::end(bfUnits_),
        [] (const Unit &unit) { return unit.isAlive() && unit.team == 1; });

    if (alive1 && alive2) {
        return Winner::NOBODY_YET;
    }
    if (alive1) {
        return Winner::TEAM_1;
    }
    if (alive2) {
        return Winner::TEAM_2;
    }

    return Winner::DRAW;
}

int GameState::addEntity(Point hex, SdlSurface img, ZOrder z)
{
    int id = numEntities();
    entities_.emplace_back(std::move(hex), std::move(img), std::move(z));

    return id;
}

int GameState::addHiddenEntity(SdlSurface img, ZOrder z)
{
    auto id = addEntity(hInvalid, std::move(img), std::move(z));
    entities_[id].visible = false;
    return id;
}

int GameState::numEntities() const
{
    return entities_.size();
}

Drawable & GameState::getEntity(int id)
{
    assert(id >= 0 && id < static_cast<int>(entities_.size()));
    return entities_[id];
}

void GameState::addUnitType(std::string name, UnitType u)
{
    unitRef_.emplace(std::move(name), std::move(u));
}

const UnitType * GameState::getUnitType(const std::string &name) const
{
    auto iter = unitRef_.find(name);
    if (iter != std::end(unitRef_)) {
        return &iter->second;
    }

    return nullptr;
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

void GameState::nextRound()
{
    auto lastAlive = stable_partition(std::begin(bfUnits_), std::end(bfUnits_),
                                      std::mem_fn(&Unit::isAlive));
    stable_sort(std::begin(bfUnits_), lastAlive, sortByInitiative);
    ++roundNum_;
}
