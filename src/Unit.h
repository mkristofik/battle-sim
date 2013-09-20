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

class UnitType;

enum class Facing { LEFT, RIGHT };

// Unit stack on the battlefield.
struct Unit
{
    int entityId;
    int num;
    int team;
    int aHex;
    Facing face;
    const UnitType *type;
    int labelId;  // entity id of text label showing number of creatures

    Unit(const UnitType &t);
};

#endif
