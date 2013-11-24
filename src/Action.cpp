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
#include "Action.h"

#include "Unit.h"
#include <cassert>
#include <ostream>

Action::Action()
    : path{},
    damage{0},
    type{ActionType::NONE},
    attacker{-1},
    defender{-1},
    aTgt{-1}
{
}

std::ostream & operator<<(std::ostream &ostr, const Action &action)
{
    /* TODO: fix this
    switch (action.type) {
        case ActionType::NONE:
            ostr << "Skip turn";
            break;
        case ActionType::MOVE:
            assert(!action.path.empty());
            ostr << "Move to " << action.path.back();
            break;
        case ActionType::ATTACK:
        {
            assert(!action.path.empty());
            assert(action.attacker);
            assert(action.defender);
            auto moveTgt = action.path.back();
            if (moveTgt != action.attacker->aHex) {
                ostr << "Move to " << moveTgt << ' ';
            }
            ostr << "Melee attack " << action.defender->getName();
            break;
        }
        case ActionType::RANGED:
            assert(action.defender);
            ostr << "Ranged attack " << action.defender->getName();
            break;
        default:
            break;
    }
    */

    return ostr;
}
