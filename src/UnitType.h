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
#ifndef UNIT_TYPE_H
#define UNIT_TYPE_H

#include "sdl_helper.h"
#include "team_color.h"

#include "rapidjson/document.h"
#include <string>
#include <unordered_map>
#include <vector>

using FrameList = std::vector<Uint32>;

// Definition of each unit type.  Contains all combat stats, descriptive info,
// images, and animations.
struct UnitType
{
    std::string name;
    std::string plural;
    int moves;
    int hp;
    bool hasRangedAttack;
    int minDmg;
    int maxDmg;
    int minDmgRanged;
    int maxDmgRanged;
    ImageSet baseImg;
    ImageSet reverseImg;
    ImageSet imgMove;
    ImageSet reverseImgMove;
    ImageSet animAttack;
    ImageSet reverseAnimAttack;
    FrameList attackFrames;
    ImageSet animRanged;
    ImageSet reverseAnimRanged;
    FrameList rangedFrames;
    SdlSurface projectile;
    ImageSet imgDefend;
    ImageSet reverseImgDefend;

    UnitType(const rapidjson::Value &json);
};

using UnitTypeMap = std::unordered_map<std::string, UnitType>;

#endif
