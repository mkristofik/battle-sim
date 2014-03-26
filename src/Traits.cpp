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

#include "algo.h"
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

    for (const auto &str : jsonListStr(json)) {
        auto i = allTraits.find(to_upper(str));
        if (i != allTraits.end()) {
            traits.push_back(i->second);
        }
        else {
            std::cerr << "Unrecognized trait: " << str << '\n';
        }
    }

    return traits;
}
