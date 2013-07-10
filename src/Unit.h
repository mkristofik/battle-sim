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

// Definition of each unit type.  Contains all combat stats, descriptive info,
// images, and animations.
struct Unit
{
    std::string name;
    std::string plural;
    int moves;
    bool hasRangedAttack;
    ImageSet baseImg;
    ImageSet reverseImg;
    ImageSet animAttack;
    ImageSet reverseAnimAttack;
    int numAttackFrames;
    ImageSet imgDefend;
    ImageSet reverseImgDefend;

    Unit(const rapidjson::Value &json);
};

using UnitsMap = std::unordered_map<std::string, Unit>;
UnitsMap parseUnits(const rapidjson::Document &doc);

#endif
