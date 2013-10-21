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
#ifndef ACTION_H
#define ACTION_H

#include <vector>

class Unit;

// Definition of a possible action a unit may take (where it can move, a target to attack, etc.).
enum class ActionType {NONE, MOVE, ATTACK, RANGED, RETALIATE};

struct Action
{
    std::vector<int> path;
    int damage;
    ActionType type;
    Unit *attacker;
    Unit *defender;

    Action();
    void computeDamage();
};

#endif
