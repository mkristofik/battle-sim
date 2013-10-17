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
#include "Unit.h"

#include "UnitType.h"
#include "algo.h"

#include "boost/lexical_cast.hpp"
#include <cstdlib>
#include <random>

Unit::Unit(const UnitType &t)
    : entityId{-1},
    num{0},
    team{-1},
    aHex{-1},
    face{Facing::RIGHT},
    type{&t},
    labelId{-1},
    hpLeft{type->hp}
{
}

int Unit::randomDamage(ActionType action) const
{
    if (action == ActionType::ATTACK) {
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

int Unit::takeDamage(int dmg)
{
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

bool Unit::isAlive() const
{
    return num > 0;
}

std::string Unit::getName(int number) const
{
    if (number == 1) {
        return "1 " + type->name;
    }
    return boost::lexical_cast<std::string>(number) + ' ' + type->plural;
}

std::string Unit::getName() const
{
    return getName(num);
}

bool sortByInitiative(const Unit &lhs, const Unit &rhs)
{
    return lhs.type->initiative > rhs.type->initiative;
}
