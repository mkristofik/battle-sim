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

#include <cassert>

std::unique_ptr<GameState> gs;

GameState::GameState()
    : entities_{},
    unitRef_{},
    bfUnits_{},
    activeUnit_{-1},
    roundNum_{0}
{
}

void GameState::nextTurn()
{
    activeUnit_ = (activeUnit_ + 1) % bfUnits_.size();

    // TODO: this will need to be more complex when units start being killed.
    if (activeUnit_ == 0) {
        ++roundNum_;
    }
}

int GameState::getRound() const
{
    return roundNum_;
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
        if (u.aHex == aIndex && u.num > 0) {
            return &u;
        }
    }

    return nullptr;
}
