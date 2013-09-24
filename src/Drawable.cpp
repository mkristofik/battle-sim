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
#include "Drawable.h"

Drawable::Drawable(Point h, SdlSurface surf, ZOrder order)
    : hex{std::move(h)},
    pOffset{0, 0},
    img{std::move(surf)},
    frame{-1},
    z{std::move(order)},
    visible{true},
    font{}
{
}

void Drawable::alignBottomCenter()
{
    pOffset.x = pHexSize / 2 - img->w / 2;
    pOffset.y = pHexSize - img->h - 1;
}
