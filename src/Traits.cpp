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
#include "Traits.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>

namespace {
    std::unordered_map<std::string, Trait> allTraits;

    void initTraits()
    {
#define X(str) allTraits.emplace(#str, Trait::str);
        UNIT_TRAITS
#undef X
    }
}

std::vector<Trait> parseTraits(const rapidjson::Value &json)
{
    if (allTraits.empty()) initTraits();

    std::vector<Trait> traits;

    std::for_each(json.Begin(), json.End(), [&] (const rapidjson::Value &elem) {
        std::string t = elem.GetString();
        std::transform(t.begin(), t.end(), t.begin(), ::toupper);
        auto i = allTraits.find(t);
        if (i != allTraits.end()) {
            traits.push_back(i->second);
        }
        else {
            std::cerr << "Unrecognized trait: " << t << '\n';
        }
    });

    return traits;
}
