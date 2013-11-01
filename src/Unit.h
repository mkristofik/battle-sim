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
#ifndef UNIT_H
#define UNIT_H

#include "Action.h"
#include <string>

class UnitType;

enum class Facing { LEFT, RIGHT };

// Unit stack on the battlefield.
struct Unit
{
    int entityId;
    int num;
    int team;
    int aHex;  // change this only via GameState::moveUnit
    Facing face;
    const UnitType *type;
    int labelId;  // entity id of text label showing number of creatures
    int hpLeft;  // hp of top creature in the stack
    bool retaliated;  // used retaliation this round

    Unit(const UnitType &t);
    int randomDamage(ActionType action) const;

    // Apply damage, return number of creatures killed.
    int takeDamage(int dmg);
    int simulateDamage(int dmg) const;

    bool isAlive() const;

    // Example: getName(5) for archer unit returns "5 Archers".
    std::string getName(int number) const;
    std::string getName() const;  // use current unit size

    int getEnemyTeam() const;
};

bool sortByInitiative(const Unit &lhs, const Unit &rhs);

#endif
