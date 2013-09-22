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
#include <random>

Unit::Unit(const UnitType &t)
    : entityId{-1},
    num{0},
    team{-1},
    aHex{-1},
    face{Facing::RIGHT},
    type{&t},
    labelId{-1}
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
