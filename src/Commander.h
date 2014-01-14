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
#ifndef COMMANDER_H
#define COMMANDER_H

#include "sdl_helper.h"

#include "rapidjson/document.h"
#include <string>

struct Commander
{
    std::string name;
    std::string alignment;
    SdlSurface portrait;
    int attack;
    int defense;

    Commander();
    Commander(const rapidjson::Value &json);
};

#endif
