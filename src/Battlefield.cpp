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
    frame{-1},
    z{std::move(order)},
    visible{true}
{
}

Battlefield::Battlefield(SDL_Rect dispArea)
    : displayArea_(std::move(dispArea)),
    entities_{},
    entityIds_{},
    grid_{5, 5},
    hexShadow_{addHiddenEntity(sdlLoadImage("hex-shadow.png"), ZOrder::SHADOW)},
    redHex_{addHiddenEntity(sdlLoadImage("hex-red.png"), ZOrder::HIGHLIGHT)},
    yellowHex_{addHiddenEntity(sdlLoadImage("hex-yellow.png"),
                               ZOrder::HIGHLIGHT)},
    greenHex_{addHiddenEntity(sdlLoadImage("hex-green.png"),
                              ZOrder::HIGHLIGHT)},
    attackSrc_{addHiddenEntity(sdlLoadImage("attack-arrow-source.png"),
                               ZOrder::HIGHLIGHT)},
    attackTgt_{addHiddenEntity(sdlLoadImage("attack-arrow-target.png"),
                               ZOrder::HIGHLIGHT)}
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

bool Battlefield::isHexValid(int aIndex) const
{
    return isHexValid(grid_.hexFromAry(aIndex));
}

int Battlefield::aryFromHex(const Point &hex) const
{
    return grid_.aryFromHex(hex);
}

Point Battlefield::hexFromAry(int aIndex) const
{
    return grid_.hexFromAry(aIndex);
}

Point Battlefield::hexFromPixel(int spx, int spy) const
{
    int mpx = spx - displayArea_.x;
    int mpy = spy - displayArea_.y;
    return ::hexFromPixel(mpx, mpy);
}

int Battlefield::aryFromPixel(int spx, int spy) const
{
    return grid_.aryFromHex(hexFromPixel(spx, spy));
}

bool Battlefield::isHexValid(const Point &hex) const
{
    return isHexValid(hex.x, hex.y);
}

std::vector<int> Battlefield::aryNeighbors(int aIndex) const
{
    if (!isHexValid(aIndex)) {
        return {};
    }

    std::vector<int> nbrs;
    for (auto d : Dir()) {
        auto aNeighbor = grid_.aryGetNeighbor(aIndex, d);
        if (isHexValid(aNeighbor)) {
            nbrs.push_back(aNeighbor);
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
    showMouseover(hexFromPixel(spx, spy));
}

void Battlefield::showMouseover(const Point &hex)
{
    if (!isHexValid(hex)) {
        hideMouseover();
        return;
    }

    entities_[hexShadow_].hex = hex;
    entities_[hexShadow_].visible = true;
}

void Battlefield::showMouseover(int aIndex)
{
    showMouseover(grid_.hexFromAry(aIndex));
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

void Battlefield::setMoveTarget(int aIndex)
{
    setMoveTarget(hexFromAry(aIndex));
}

void Battlefield::clearMoveTarget()
{
    entities_[greenHex_].visible = false;
}

void Battlefield::setRangedTarget(const Point &hex)
{
    if (!isHexValid(hex)) {
        clearRangedTarget();
        return;
    }

    entities_[redHex_].hex = hex;
    entities_[redHex_].visible = true;
}

void Battlefield::clearRangedTarget()
{
    entities_[redHex_].visible = false;
}

void Battlefield::showAttackArrow(int spx, int spy)
{
    // TODO: this should really be a user function, especially since we want to
    // override the mouseover shadow.  The real function here should just be to
    // draw the arrow from attack source to target.

    auto tgtHex = hexFromPixel(spx, spy);
    if (!isHexValid(tgtHex)) {
        hideAttackArrow();
        return;
    }

    // Compute where the location is within the target hex.  Direction in this
    // case is where the attack is coming from (like wind direction).
    auto spTgtHex = sPixelFromHex(tgtHex);
    auto xOffset = spx - spTgtHex.x;
    auto yOffset = spy - spTgtHex.y;
    auto attackDir = getSector(xOffset, yOffset);

    auto srcHex = adjacent(tgtHex, attackDir);
    if (!isHexValid(srcHex)) {
        hideAttackArrow();
        return;
    }

    auto dir = static_cast<int>(attackDir);
    entities_[attackSrc_].hex = srcHex;
    entities_[attackSrc_].frame = dir;
    entities_[attackSrc_].visible = true;
    entities_[attackTgt_].hex = tgtHex;
    entities_[attackTgt_].frame = dir;
    entities_[attackTgt_].visible = true;

    // Override the mouseover shadow.  It looks better to highlight the hex
    // you're going to attack from.
    showMouseover(srcHex);
}

void Battlefield::showAttackArrow2(int aSrc, int aTgt)
{
    auto hSrc = hexFromAry(aSrc);
    auto hTgt = hexFromAry(aTgt);
    auto attackDir = static_cast<int>(direction(hTgt, hSrc));
    entities_[attackSrc_].hex = hSrc;
    entities_[attackSrc_].frame = attackDir;
    entities_[attackSrc_].visible = true;
    entities_[attackTgt_].hex = hTgt;
    entities_[attackTgt_].frame = attackDir;
    entities_[attackTgt_].visible = true;
}

void Battlefield::hideAttackArrow()
{
    entities_[attackSrc_].visible = false;
    entities_[attackTgt_].visible = false;
}

void Battlefield::clearHighlights()
{
    hideMouseover();
    clearMoveTarget();
    clearRangedTarget();
    hideAttackArrow();
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
                if (e.frame < 0) {
                    sdlBlit(e.img, sPixelFromHex(e.hex) + e.pOffset);
                }
                else {
                    sdlBlitFrame(e.img, e.frame,
                                 sPixelFromHex(e.hex) + e.pOffset);
                }
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
