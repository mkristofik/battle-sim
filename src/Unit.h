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
#ifndef UNIT_H
#define UNIT_H

#include "Action.h"
#include "Effects.h"
#include "Traits.h"
#include <string>

class UnitType;

enum class Facing { LEFT, RIGHT };

// TODO - ideas to make this smaller
// - store facing with the Drawable
// - store labelId in the Battlefield

// Unit stack on the battlefield.
struct Unit
{
    Effect effect;  // might want more than one of these
    int entityId;
    int num;
    int team;
    int aHex;  // change this only via GameState::moveUnit
    Facing face;  // the only field Animations are allowed to change
    const UnitType *type;
    int labelId;  // entity id of text label showing number of creatures
    int hpLeft;  // hp of top creature in the stack
    bool retaliated;  // used retaliation this round

    Unit();
    explicit Unit(const UnitType &t);

    int randomDamage(ActionType action) const;
    int avgDamage(ActionType action) const;

    // Apply damage, return number of creatures killed.
    int takeDamage(int dmg);
    int simulateDamage(int dmg) const;

    bool isValid() const;  // return false if default-constructed
    bool isAlive() const;

    // Example: getName(5) for archer unit returns "5 Archers".
    std::string getName(int number) const;
    std::string getName() const;  // use current unit size

    bool isEnemy(const Unit &other) const;

    // Maximum distance this unit can move in one turn.  Note that Pathfinder
    // includes the hex the unit is standing in when building paths.
    unsigned getMaxPathSize() const;

    bool hasTrait(Trait t) const;
    bool hasEffect(EffectType e) const;
    bool canFly() const;  // prefer this over hasTrait(Trait::FLYING)
};

bool sortByInitiative(const Unit &lhs, const Unit &rhs);

#endif
