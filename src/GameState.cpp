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
    activeUnit_{-1},
    roundNum_{1}
{
}

void GameState::nextTurn()
{
    auto nextUnit = activeUnit_;
    int totalUnits = bfUnits_.size();
    int numTried = 0;

    // Loop through the units until we find one that is still alive.
    while (numTried < totalUnits) {
        ++nextUnit;
        if (nextUnit >= totalUnits) {
            ++roundNum_;
            nextUnit = 0;
        }
        if (bfUnits_[nextUnit].isAlive()) {
            activeUnit_ = nextUnit;
            return;
        }
        ++numTried;
    }

    activeUnit_ = -1;
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
}

Unit * GameState::getActiveUnit()
{
    if (activeUnit_ < 0) {
        return nullptr;
    }

    return &bfUnits_[activeUnit_];
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
