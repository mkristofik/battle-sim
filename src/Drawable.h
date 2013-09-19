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
#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "hex_utils.h"
#include "sdl_helper.h"

enum class ZOrder {TERRAIN,
                   GRID,
                   CREATURE,
                   SHADOW,
                   HIGHLIGHT,
                   PROJECTILE,
                   ANIMATING};

struct Drawable
{
    Point hex;
    Point pOffset;
    SdlSurface img;
    int frame;
    ZOrder z;
    bool visible;

    Drawable(Point h, SdlSurface surf, ZOrder order);

    // Align the image with the bottom-center of its hex.
    void alignBottomCenter();
};

#endif
