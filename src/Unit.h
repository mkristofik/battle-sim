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

#include "team_color.h"

#include "rapidjson/document.h"
#include <string>
#include <unordered_map>

enum class Facing { LEFT, RIGHT };

// Definition of each unit type.  Contains all combat stats, descriptive info,
// images, and animations.
struct UnitType
{
    std::string name;
    std::string plural;
    int moves;
    bool hasRangedAttack;
    ImageSet baseImg;
    ImageSet reverseImg;
    ImageSet imgMove;
    ImageSet reverseImgMove;
    ImageSet animAttack;
    ImageSet reverseAnimAttack;
    int numAttackFrames;
    ImageSet imgDefend;
    ImageSet reverseImgDefend;

    UnitType(const rapidjson::Value &json);
};

// Unit stack on the battlefield.
struct Unit
{
    int entityId;
    int num;
    int team;
    int aHex;
    const UnitType *type;

    Unit() : entityId{-1}, num{0}, team{-1}, aHex{-1}, type{nullptr} {}
};

using UnitTypeMap = std::unordered_map<std::string, UnitType>;

#endif
