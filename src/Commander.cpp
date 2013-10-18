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
#include "Commander.h"

Commander::Commander()
    : name{"Captain"},
    alignment{"Neutral"},
    portrait{sdlLoadImage("portrait-captain.png")},
    attack{0},
    defense{0}
{
}

Commander::Commander(const rapidjson::Value &json)
    : name{},
    alignment{},
    portrait{},
    attack{0},
    defense{0}
{
    if (json.HasMember("name")) {
        name = json["name"].GetString();
    }
    if (json.HasMember("alignment")) {
        alignment = json["alignment"].GetString();
    }
    if (json.HasMember("portrait")) {
        portrait = sdlLoadImage(json["portrait"].GetString());
    }
    if (json.HasMember("attack")) {
        attack = json["attack"].GetInt();
    }
    if (json.HasMember("defense")) {
        defense = json["defense"].GetInt();
    }
}
