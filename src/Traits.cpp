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
    std::unordered_map<int, std::string> traitStr;

    void initTraits()
    {
#define X(str) allTraits.emplace(#str, Trait::str);
        UNIT_TRAITS
#undef X
        for (const auto &i : allTraits) {
            auto str = i.first;
            auto idx = str.find('_');
            while (idx != std::string::npos) {
                str.replace(idx, 1, " ");
                idx = str.find('_', idx + 1);
            }

            traitStr.emplace(static_cast<int>(i.second), to_lower(str));
        }
    }
}

bool operator<(Trait lhs, Trait rhs)
{
    return static_cast<int>(lhs) < static_cast<int>(rhs);
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

std::string strFromTraits(const std::vector<Trait> &traits)
{
    if (traits.empty()) return {};

    std::string ret;
    for (auto t : traits) {
        ret += traitStr[static_cast<int>(t)];
        ret += ", ";
    }
    ret.erase(ret.size() - 2);
    return ret;
}
