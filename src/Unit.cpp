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
#include "Unit.h"

#include "UnitType.h"
#include "algo.h"

#include "boost/lexical_cast.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <random>

Unit::Unit()
    : effect{},
    entityId{-1},
    num{0},
    team{-1},
    aHex{-1},
    face{Facing::RIGHT},
    type{nullptr},
    labelId{-1},
    hpLeft{0},
    retaliated{false}
{
}

Unit::Unit(const UnitType &t)
    : Unit()
{
    type = &t;
    hpLeft = type->hp;
}

int Unit::randomDamage(ActionType action) const
{
    if (!isValid()) return 0;

    if (action == ActionType::ATTACK || action == ActionType::RETALIATE) {
        std::uniform_int_distribution<int> dmg(type->minDmg, type->maxDmg);
        return dmg(randomGenerator());
    }
    else if (action == ActionType::RANGED) {
        std::uniform_int_distribution<int> dmg(type->minDmgRanged,
                                               type->maxDmgRanged);
        return dmg(randomGenerator());
    }

    return 0;
}

int Unit::avgDamage(ActionType action) const
{
    if (!isValid()) return 0;

    if (action == ActionType::ATTACK || action == ActionType::RETALIATE) {
        return nearbyint((type->minDmg + type->maxDmg) / 2);
    }
    else if (action == ActionType::RANGED) {
        return nearbyint((type->minDmgRanged + type->maxDmgRanged) / 2);
    }

    return 0;
}

int Unit::takeDamage(int dmg)
{
    if (!isValid()) return 0;

    if (dmg < hpLeft) {
        hpLeft -= dmg;
        return 0;
    }

    int startSize = num;

    // Remove the top creature in the stack.
    dmg -= hpLeft;
    hpLeft = type->hp;
    --num;

    // Remove as many whole creatures as possible.
    auto result = div(dmg, type->hp);
    num -= result.quot;
    if (num < 0) {
        num = 0;
        hpLeft = 0;
        return startSize;
    }

    // Damage the top remaining creature.
    hpLeft -= result.rem;
    return startSize - num;
}

int Unit::simulateDamage(int dmg) const
{
    return Unit(*this).takeDamage(dmg);
}

bool Unit::isValid() const
{
    return entityId >= 0 && team >= 0 && type;
}

bool Unit::isAlive() const
{
    return isValid() && num > 0;
}

std::string Unit::getName(int number) const
{
    if (!isValid()) return {};

    if (number == 1) {
        return "1 " + type->name;
    }
    return boost::lexical_cast<std::string>(number) + ' ' + type->plural;
}

std::string Unit::getName() const
{
    return getName(num);
}

bool Unit::isEnemy(const Unit &other) const
{
    return team != other.team;
}

unsigned Unit::getMaxPathSize() const
{
    if (!isValid()) return 0;

    if (effect.type == EffectType::BOUND) {
        return 1;
    }
    return static_cast<unsigned>(type->moves) + 1;
}

bool Unit::hasTrait(Trait t) const
{
    if (!isValid()) return false;
    return contains(type->traits, t);
}

bool sortByInitiative(const Unit &lhs, const Unit &rhs)
{
    assert(lhs.type && rhs.type);
    return lhs.type->initiative > rhs.type->initiative;
}
