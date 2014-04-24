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
#ifndef UNIT_VIEW_H
#define UNIT_VIEW_H

#include "sdl_helper.h"

class Unit;

/*
 * -------
 * |     |  23 Archers
 * |     |  Dmg 2-3
 * |     |  HP 8/10
 * -------
 *  Trait 1, Trait 2, ...
 *  Status 1, Status 2, ...
 */
SdlSurface renderUnitView(const Unit &unit);

#endif
