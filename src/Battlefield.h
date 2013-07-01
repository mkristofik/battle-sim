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
#ifndef BATTLEFIELD_H
#define BATTLEFIELD_H

#include "hex_utils.h"
#include "sdl_helper.h"

// Handle the drawing of the hexagonal battlefield and all of the drawable
// entities on the battlefield.

struct Drawable
{
    Point hex;
    Point pOffset;
    SdlSurface img;

    Drawable();
    Drawable(Point h, SdlSurface surf);
};

class Battlefield
{
public:
    Battlefield(const SDL_Rect &dispArea);

    bool isHexValid(int hx, int hy) const;
    bool isHexValid(const Point &hex) const;

    // Add a drawable entity to the battlefield.  Return its unique id number.
    int addEntity(Point hex, SdlSurface img);

    void draw();

private:
    // Return the screen coordinates of a hex.
    Point sPixelFromHex(int hx, int hy) const;
    Point sPixelFromHex(const Point &hex) const;

    SDL_Rect displayArea_;
    SdlSurface tile_;
    SdlSurface grid_;
    std::vector<Drawable> entities_;
};

#endif
