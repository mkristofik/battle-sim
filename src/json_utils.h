/*
    Copyright (C) 2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "rapidjson/document.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

// Load a JSON document into 'doc', return true if successful.
bool jsonParse(const char *filename, rapidjson::Document &doc);

std::vector<unsigned> jsonListUnsigned(const rapidjson::Value &json);
std::vector<std::string> jsonListStr(const rapidjson::Value &json);

#endif
