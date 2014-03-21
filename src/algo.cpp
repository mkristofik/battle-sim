/*
    Copyright (C) 2012-2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "algo.h"
#include <cctype>
#include <ctime>

std::minstd_rand & randomGenerator()
{
    static std::minstd_rand gen(static_cast<unsigned int>(std::time(nullptr)));
    return gen;
}

std::string to_upper(std::string str)
{
    transform(std::begin(str), std::end(str), std::begin(str), ::toupper);
    return str;
}

std::string to_lower(std::string str)
{
    transform(std::begin(str), std::end(str), std::begin(str), ::tolower);
    return str;
}
