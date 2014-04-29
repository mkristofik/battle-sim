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
#ifndef TRAITS_H
#define TRAITS_H

#include "json_utils.h"
#include <string>
#include <vector>

#define UNIT_TRAITS \
    X(BINDING) \
    X(DOUBLE_STRIKE) \
    X(FIRST_STRIKE) \
    X(FLYING) \
    X(LIFE_DRAIN) \
    X(MOUNTED) \
    X(RANGED) \
    X(REGENERATE) \
    X(SPELLCASTER) \
    X(STEADFAST) \
    X(TRAMPLE) \
    X(ZOMBIFY)

#define X(str) str,
enum class Trait {UNIT_TRAITS};
#undef X
bool operator<(Trait lhs, Trait rhs);

std::vector<Trait> parseTraits(const rapidjson::Value &json);

// Convert a list of traits into a string separated by commas.
std::string strFromTraits(const std::vector<Trait> &traits);

#endif
