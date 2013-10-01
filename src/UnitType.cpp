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
#include "UnitType.h"
#include <algorithm>
#include <iterator>

namespace
{
    void loadImages(const rapidjson::Value &json,
                    const char *imgName,
                    ImageSet &img,
                    ImageSet &reverseImg)
    {
        auto tempImg = sdlLoadImage(json[imgName].GetString());
        if (tempImg) {
            img = applyTeamColors(tempImg);
            reverseImg = applyTeamColors(sdlFlipH(tempImg));
        }
    }

    void loadAnimation(const rapidjson::Value &json,
                       const char *animName,
                       ImageSet &anim,
                       ImageSet &reverseAnim,
                       const char *framesName,
                       FrameList &frames)
    {
        const auto &frameListJson = json[framesName];
        transform(frameListJson.Begin(),
                  frameListJson.End(),
                  std::back_inserter(frames),
                  [&] (const rapidjson::Value &elem) { return elem.GetInt(); });

        auto baseAnim = sdlLoadImage(json[animName].GetString());
        if (baseAnim) {
            anim = applyTeamColors(baseAnim);
            reverseAnim = applyTeamColors(sdlFlipSheetH(baseAnim,
                frames.size()));
        }
    }
}

UnitType::UnitType(const rapidjson::Value &json)
    : name{},
    plural{},
    moves{1},
    initiative{0},
    hp{1},
    hasRangedAttack{false},
    minDmg{1},
    maxDmg{1},
    minDmgRanged{0},
    maxDmgRanged{0},
    baseImg{},
    reverseImg{},
    animAttack{},
    reverseAnimAttack{},
    attackFrames{},
    animRanged{},
    reverseAnimRanged{},
    rangedFrames{},
    projectile{},
    imgDefend{},
    reverseImgDefend{},
    animDie{},
    reverseAnimDie{},
    dieFrames{}
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
    if (json.HasMember("initiative")) {
        initiative = json["initiative"].GetInt();
    }
    if (json.HasMember("hp")) {
        hp = json["hp"].GetInt();
    }
    if (json.HasMember("img")) {
        loadImages(json, "img", baseImg, reverseImg);
    }
    if (json.HasMember("img-move")) {
        loadImages(json, "img-move", imgMove, reverseImgMove);
    }
    if (json.HasMember("img-defend")) {
        loadImages(json, "img-defend", imgDefend, reverseImgDefend);
    }
    if (json.HasMember("anim-attack") && json.HasMember("attack-frames")) {
        loadAnimation(json, "anim-attack", animAttack, reverseAnimAttack,
                      "attack-frames", attackFrames);
    }
    if (json.HasMember("anim-ranged") && json.HasMember("ranged-frames")) {
        loadAnimation(json, "anim-ranged", animRanged, reverseAnimRanged,
                      "ranged-frames", rangedFrames);
    }
    if (json.HasMember("damage")) {
        const auto &damageList = json["damage"];
        minDmg = damageList[0u].GetInt();
        maxDmg = damageList[1u].GetInt();
    }
    if (json.HasMember("damage-ranged")) {
        hasRangedAttack = true;

        const auto &damageList = json["damage-ranged"];
        minDmgRanged = damageList[0u].GetInt();
        maxDmgRanged = damageList[1u].GetInt();
    }
    if (json.HasMember("projectile")) {
        projectile = sdlLoadImage(json["projectile"].GetString());
    }
    if (json.HasMember("anim-die") && json.HasMember("die-frames")) {
        loadAnimation(json, "anim-die", animDie, reverseAnimDie, "die-frames",
                      dieFrames);
    }
}
