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

Drawable::Drawable(Point h, SdlSurface surf)
    : hex{std::move(h)},
    pOffset{0, 0},
    img{std::move(surf)},
    visible{true}
{
}

Battlefield::Battlefield(SDL_Rect dispArea)
    : displayArea_(std::move(dispArea)),
    entities_{},
    tile_{},
    grid_{},
    hexShadow_{-1},
    redHex_{-1},
    yellowHex_{-1},
    greenHex_{-1}
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

int Battlefield::addEntity(Point hex, SdlSurface img)
{
    entities_.emplace_back(std::move(hex), std::move(img));
    return entities_.size() - 1;
}

void Battlefield::showMouseover(int spx, int spy)
{
    int mpx = spx - displayArea_.x;
    int mpy = spy - displayArea_.y;
    auto hex = hexFromPixel(mpx, mpy);

    if (!isHexValid(hex)) {
        hideMouseover();
        return;
    }

    if (hexShadow_ < 0) {
        hexShadow_ = addEntity(hex, sdlLoadImage("hex-shadow.png"));
    }

    entities_[hexShadow_].hex = hex;
    entities_[hexShadow_].visible = true;
}

void Battlefield::hideMouseover()
{
    if (hexShadow_ >= 0) {
        entities_[hexShadow_].visible = false;
    }
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
                auto sp = sPixelFromHex(hx, hy);
                sdlBlit(tile_, sp);
                if (isHexValid(hx, hy)) {
                    sdlBlit(grid_, sp);
                }
            }
        }

        for (const auto &e : entities_) {
            if (e.visible) {
                sdlBlit(e.img, sPixelFromHex(e.hex) + e.pOffset);
            }
        }
    });
}

Point Battlefield::sPixelFromHex(int hx, int hy) const
{
    auto basePixel = pixelFromHex(hx, hy);
    int px = basePixel.x + displayArea_.x;
    int py = basePixel.y + displayArea_.y;

    return {px, py};
}

Point Battlefield::sPixelFromHex(const Point &hex) const
{
    return sPixelFromHex(hex.x, hex.y);
}
