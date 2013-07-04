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
#include <algorithm>
#include <cassert>

Drawable::Drawable(Point h, SdlSurface surf, ZOrder order)
    : hex{std::move(h)},
    pOffset{0, 0},
    img{std::move(surf)},
    z{std::move(order)},
    visible{true}
{
}

Battlefield::Battlefield(SDL_Rect dispArea)
    : displayArea_(std::move(dispArea)),
    entities_{},
    entityIds_{},
    hexShadow_{addHiddenEntity(sdlLoadImage("hex-shadow.png"),
                               ZOrder::HIGHLIGHT)},
    redHex_{addHiddenEntity(sdlLoadImage("hex-red.png"), ZOrder::HIGHLIGHT)},
    yellowHex_{addHiddenEntity(sdlLoadImage("hex-yellow.png"),
                               ZOrder::HIGHLIGHT)},
    greenHex_{addHiddenEntity(sdlLoadImage("hex-green.png"), ZOrder::HIGHLIGHT)}
{
    auto tile = sdlLoadImage("grass.png");
    auto grid = sdlLoadImage("hex-grid.png");

    // Create the background terrain and hex grid.  They can be treated as
    // drawable entities like everything else.
    for (int hx = -1; hx <= 5; ++hx) {
        for (int hy = -1; hy <= 5; ++hy) {
            Point hex = {hx, hy};
            addEntity(hex, tile, ZOrder::TERRAIN);
            if (isHexValid(hex)) {
                addEntity(hex, grid, ZOrder::GRID);
            }
        }
    }
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

Point Battlefield::hexFromPixel(int spx, int spy) const
{
    int mpx = spx - displayArea_.x;
    int mpy = spy - displayArea_.y;
    return ::hexFromPixel(mpx, mpy);
}

bool Battlefield::isHexValid(const Point &hex) const
{
    return isHexValid(hex.x, hex.y);
}

std::vector<Point> Battlefield::hexNeighbors(const Point &hex) const
{
    if (!isHexValid(hex)) {
        return {};
    }

    std::vector<Point> nbrs;
    for (auto d : Dir()) {
        Point n = adjacent(hex, d);
        if (isHexValid(n)) {
            nbrs.emplace_back(std::move(n));
        }
    }

    return nbrs;
}

int Battlefield::addEntity(Point hex, SdlSurface img, ZOrder z)
{
    int id = entities_.size();
    entities_.emplace_back(std::move(hex), std::move(img), std::move(z));
    entityIds_.push_back(id);

    return id;
}

int Battlefield::addHiddenEntity(SdlSurface img, ZOrder z)
{
    auto id = addEntity(hInvalid, std::move(img), std::move(z));
    entities_[id].visible = false;
    return id;
}

const Drawable & Battlefield::getEntity(int id) const
{
    assert(id >= 0 && id < static_cast<int>(entities_.size()));
    return entities_[id];
}

void Battlefield::showMouseover(int spx, int spy)
{
    auto hex = hexFromPixel(spx, spy);
    if (!isHexValid(hex)) {
        hideMouseover();
        return;
    }

    entities_[hexShadow_].hex = hex;
    entities_[hexShadow_].visible = true;
}

void Battlefield::hideMouseover()
{
    entities_[hexShadow_].visible = false;
}

void Battlefield::selectEntity(int id)
{
    const auto &hex = getEntity(id).hex;
    entities_[yellowHex_].hex = hex;
    entities_[yellowHex_].visible = true;
}

void Battlefield::deselectEntity()
{
    entities_[yellowHex_].visible = false;
}

void Battlefield::setMoveTarget(const Point &hex)
{
    if (!isHexValid(hex)) {
        clearMoveTarget();
        return;
    }

    entities_[greenHex_].hex = hex;
    entities_[greenHex_].visible = true;
}

void Battlefield::clearMoveTarget()
{
    entities_[greenHex_].visible = false;
}

void Battlefield::draw()
{
    // Arrange entities logically by z-order.
    sort(std::begin(entityIds_), std::end(entityIds_), [&] (int a, int b)
         { return entities_[a].z < entities_[b].z; });

    sdlClear(displayArea_);
    SdlSetClipRect(displayArea_, [&] {
        for (auto id : entityIds_) {
            const auto &e = entities_[id];
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
