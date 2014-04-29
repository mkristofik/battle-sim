/*
    Copyright (C) 2013-2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "UnitType.h"
#include "algo.h"
#include <algorithm>
#include <iostream>

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
        frames = jsonListUnsigned(json[framesName]);

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
    minDmg{0},
    maxDmg{0},
    minDmgRanged{0},
    maxDmgRanged{0},
    growth{1},
    traits{},
    spell{nullptr},
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
    bool hasRangedAttack = false;

    if (json.HasMember("name")) {
        name = json["name"].GetString();
    }
    if (json.HasMember("plural")) {
        plural = json["plural"].GetString();
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
    if (json.HasMember("growth")) {
        growth = json["growth"].GetInt();
    }
    if (json.HasMember("projectile")) {
        projectile = sdlLoadImage(json["projectile"].GetString());
    }
    if (json.HasMember("anim-die") && json.HasMember("die-frames")) {
        loadAnimation(json, "anim-die", animDie, reverseAnimDie, "die-frames",
                      dieFrames);
    }
    if (json.HasMember("traits")) {
        auto tlist = parseTraits(json["traits"]);
        traits.insert(std::end(traits), std::begin(tlist), std::end(tlist));
    }
    if (json.HasMember("spell")) {
        spell = getSpell(json["spell"].GetString());
        if (spell) {
            traits.push_back(Trait::SPELLCASTER);
        }
        else {
            std::cerr << "WARNING: unrecognized spell for " << name << '\n';
        }
    }

    if (hasRangedAttack) {
        traits.push_back(Trait::RANGED);
        if (!projectile) {
            std::cerr << "WARNING: projectile not specified for " <<
                name << '\n';
            projectile = sdlLoadImage("missile.png");
        }
    }

    if (contains(traits, Trait::MOUNTED)) {
        moves = 2;
    }

    sort(std::begin(traits), std::end(traits));
}
