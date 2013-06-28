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
#include "sdl_helper.h"

#include <cstdio>
#include <iostream>
#include <memory>

namespace {
    Unit loadUnit(const rapidjson::Value &json)
    {
        Unit u;
        if (json.HasMember("name")) {
            u.name = json["name"].GetString();
        }
        if (json.HasMember("plural")) {
            u.plural = json["plural"].GetString();
        }
        if (json.HasMember("img")) {
            auto baseImg = sdlLoadImage(json["img"].GetString());
            if (baseImg) {
                u.baseImg = applyTeamColors(baseImg);
                u.reverseImg = applyTeamColors(sdlFlipH(baseImg));
            }
        }
        if (json.HasMember("img-defend")) {
            auto baseImg = sdlLoadImage(json["img-defend"].GetString());
            if (baseImg) {
                u.imgDefend = applyTeamColors(baseImg);
                u.reverseImgDefend = applyTeamColors(sdlFlipH(baseImg));
            }
        }
        if (json.HasMember("anim-attack")) {
            auto baseAnim = sdlLoadImage(json["anim-attack"].GetString());
            if (baseAnim) {
                u.animAttack = applyTeamColors(baseAnim);

                // Assume each animation frame is sized to fit the standard hex.
                int n = baseAnim->w / pHexSize;
                u.numAttackFrames = n;
                u.reverseAnimAttack = applyTeamColors(sdlFlipSheetH(baseAnim,
                                                                    n));
            }
        }
        if (json.HasMember("anim-die")) {
        }

        return u;
    }
}

UnitsMap parseUnits(const rapidjson::Document &doc)
{
    UnitsMap um;
    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "units: skipping id '" << i->name.GetString() <<
                "'\n";
            continue;
        }
        um.emplace(i->name.GetString(), loadUnit(i->value));
    }

    return um;
}
