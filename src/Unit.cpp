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
#include <iostream>

UnitType::UnitType(const rapidjson::Value &json)
    : name{},
    plural{},
    moves{1},
    hasRangedAttack{false},
    baseImg{},
    reverseImg{},
    animAttack{},
    attackFrames{},
    reverseAnimAttack{},
    imgDefend{},
    reverseImgDefend{}
{
    if (json.HasMember("name")) {
        name = json["name"].GetString();
    }
    if (json.HasMember("plural")) {
        plural = json["plural"].GetString();
    }
    if (json.HasMember("moves")) {
        moves = json["moves"].GetInt();
    }
    if (json.HasMember("ranged")) {
        hasRangedAttack = (json["ranged"].GetInt() != 0);
    }
    if (json.HasMember("img")) {
        auto img = sdlLoadImage(json["img"].GetString());
        if (img) {
            baseImg = applyTeamColors(img);
            reverseImg = applyTeamColors(sdlFlipH(img));
        }
    }
    if (json.HasMember("img-move")) {
        auto img = sdlLoadImage(json["img-move"].GetString());
        if (img) {
            imgMove = applyTeamColors(img);
            reverseImgMove = applyTeamColors(sdlFlipH(img));
        }
    }
    if (json.HasMember("img-defend")) {
        auto img = sdlLoadImage(json["img-defend"].GetString());
        if (img) {
            imgDefend = applyTeamColors(img);
            reverseImgDefend = applyTeamColors(sdlFlipH(img));
        }
    }
    if (json.HasMember("anim-attack")) {
        auto baseAnim = sdlLoadImage(json["anim-attack"].GetString());
        if (baseAnim) {
            animAttack = applyTeamColors(baseAnim);

            // Assume each animation frame is sized to fit the standard hex.
            int numAttackFrames = baseAnim->w / pHexSize;
            reverseAnimAttack = applyTeamColors(sdlFlipSheetH(baseAnim,
                                                              numAttackFrames));
        }
    }
    if (json.HasMember("attack-frames")) {
        const auto &frameList = json["attack-frames"];
        auto iter = frameList.Begin();
        while (iter != frameList.End()) {
            attackFrames.push_back(iter->GetInt());
            ++iter;
        }
    }
    if (json.HasMember("anim-die")) {
    }
}

Unit::Unit()
    : entityId{-1},
    num{0},
    team{-1},
    aHex{-1},
    face{Facing::RIGHT},
    type{nullptr}
{
}
