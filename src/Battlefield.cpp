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
#include "Battlefield.h"

Battlefield::Battlefield(const SDL_Rect &dispArea)
    : displayArea_(dispArea),
    tile_{},
    grid_{}
{
}

bool Battlefield::isHexValid(int hx, int hy) const
{
    // Battlefield is shaped like a hexagon.  It's a grid with the corners cut
    // off.
    if ((hx >= 0 && hx < 5) && (hy >= 1 && hy < 4))
    {
        return true;
    }
    else if ((hx >= 1 && hx < 4) && (hy == 0))
    {
        return true;
    }
    else if (hx == 2 && hy == 4)
    {
        return true;
    }

    return false;
}

bool Battlefield::isHexValid(const Point &hex) const
{
    return isHexValid(hex.x, hex.y);
}

void Battlefield::draw()
{
    if (!tile_ || !grid_) {
        tile_ = sdlLoadImage("grass.png");
        grid_ = sdlLoadImage("hex-grid.png");
    }

    sdlClear(displayArea_);
    SdlSetClipRect(displayArea_, [&] {
        for (int hx = -1; hx <= 5; ++hx) {
            for (int hy = -1; hy <= 5; ++hy) {
                auto basePixel = pixelFromHex(hx, hy);
                int px = basePixel.x + displayArea_.x;
                int py = basePixel.y + displayArea_.y;

                sdlBlit(tile_, px, py);
                if (isHexValid(hx, hy)) {
                    sdlBlit(grid_, px, py);
                }
            }
        }
    });
}
