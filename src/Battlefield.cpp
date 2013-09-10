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

#include "Action.h"
#include "Drawable.h"
#include "GameState.h"

#include "boost/lexical_cast.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>

Battlefield::Battlefield(SDL_Rect dispArea)
    : displayArea_(std::move(dispArea)),
    grid_{5, 5},
    hexShadow_{gs->addHiddenEntity(sdlLoadImage("hex-shadow.png"),
                                   ZOrder::SHADOW)},
    redHex_{gs->addHiddenEntity(sdlLoadImage("hex-red.png"),
                                ZOrder::HIGHLIGHT)},
    yellowHex_{gs->addHiddenEntity(sdlLoadImage("hex-yellow.png"),
                                   ZOrder::HIGHLIGHT)},
    greenHex_{gs->addHiddenEntity(sdlLoadImage("hex-green.png"),
                                  ZOrder::HIGHLIGHT)},
    attackSrc_{gs->addHiddenEntity(sdlLoadImage("attack-arrow-source.png"),
                                   ZOrder::HIGHLIGHT)},
    attackTgt_{gs->addHiddenEntity(sdlLoadImage("attack-arrow-target.png"),
                                   ZOrder::HIGHLIGHT)},
    font_{sdlLoadFont("../DejaVuSans.ttf", 8)}
{
    auto tile = sdlLoadImage("grass.png");
    auto grid = sdlLoadImage("hex-grid.png");

    // Create the background terrain and hex grid.  They can be treated as
    // drawable entities like everything else.
    for (int hx = -1; hx <= 5; ++hx) {
        for (int hy = -1; hy <= 5; ++hy) {
            Point hex = {hx, hy};
            gs->addEntity(hex, tile, ZOrder::TERRAIN);
            if (isHexValid(hex)) {
                gs->addEntity(hex, grid, ZOrder::GRID);
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

void Battlefield::handleMouseMotion(const SDL_MouseMotionEvent &event,
                                    const Action &action)
{
    clearHighlights();
    if (action.type == ActionType::ATTACK) {
        auto aMoveTo = action.path.back();
        showMouseover(aMoveTo);
        showAttackArrow(aMoveTo, action.attackTarget);
    }
    else if (action.type == ActionType::RANGED) {
        showMouseover(action.attackTarget);
        setRangedTarget(action.attackTarget);
    }
    else if (action.type == ActionType::MOVE) {
        auto aMoveTo = action.path.back();
        showMouseover(aMoveTo);
        setMoveTarget(aMoveTo);
    }
    else {
        showMouseover(event.x, event.y);
    }
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

    auto &shadow = gs->getEntity(hexShadow_);
    shadow.hex = hex;
    shadow.visible = true;
}

void Battlefield::showMouseover(int aIndex)
{
    showMouseover(grid_.hexFromAry(aIndex));
}

void Battlefield::hideMouseover()
{
    gs->getEntity(hexShadow_).visible = false;
}

void Battlefield::selectHex(int aIndex)
{
    auto &selected = gs->getEntity(yellowHex_);
    selected.hex = hexFromAry(aIndex);
    selected.visible = true;
}

void Battlefield::deselectHex()
{
    gs->getEntity(yellowHex_).visible = false;
}

void Battlefield::setMoveTarget(const Point &hex)
{
    if (!isHexValid(hex)) {
        clearMoveTarget();
        return;
    }

    auto &target = gs->getEntity(greenHex_);
    target.hex = hex;
    target.visible = true;
}

void Battlefield::setMoveTarget(int aIndex)
{
    setMoveTarget(hexFromAry(aIndex));
}

void Battlefield::clearMoveTarget()
{
    gs->getEntity(greenHex_).visible = false;
}

void Battlefield::setRangedTarget(const Point &hex)
{
    if (!isHexValid(hex)) {
        clearRangedTarget();
        return;
    }

    auto &target = gs->getEntity(redHex_);
    target.hex = hex;
    target.visible = true;
}

void Battlefield::setRangedTarget(int aIndex)
{
    setRangedTarget(hexFromAry(aIndex));
}

void Battlefield::clearRangedTarget()
{
    gs->getEntity(redHex_).visible = false;
}

void Battlefield::showAttackArrow(int aSrc, int aTgt)
{
    auto hSrc = hexFromAry(aSrc);
    auto hTgt = hexFromAry(aTgt);
    auto &source = gs->getEntity(attackSrc_);
    auto &target = gs->getEntity(attackTgt_);

    // Direction graphics are arranged like the wind; we want the direction the
    // attack is coming from.
    auto attackDir = static_cast<int>(direction(hTgt, hSrc));

    source.hex = hSrc;
    source.frame = attackDir;
    source.visible = true;
    target.hex = hTgt;
    target.frame = attackDir;
    target.visible = true;
}

void Battlefield::hideAttackArrow()
{
    gs->getEntity(attackSrc_).visible = false;
    gs->getEntity(attackTgt_).visible = false;
}

void Battlefield::clearHighlights()
{
    hideMouseover();
    clearMoveTarget();
    clearRangedTarget();
    hideAttackArrow();
}

void Battlefield::draw() const
{
    std::vector<int> entityIds(gs->numEntities());
    iota(std::begin(entityIds), std::end(entityIds), 0);

    // Arrange entities logically by z-order.
    sort(std::begin(entityIds), std::end(entityIds), [&] (int a, int b)
         { return gs->getEntity(a).z < gs->getEntity(b).z; });

    sdlClear(displayArea_);
    SdlSetClipRect(displayArea_, [&] {
        for (auto id : entityIds) {
            const auto &e = gs->getEntity(id);
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

        // This is O(n^2) but is probably good enough.
        for (int i = 0; i < grid_.size(); ++i) {
            auto unit = gs->getUnitAt(i);
            if (!unit || unit->num <= 0) continue;

            // Draw label centered at the bottom of the hex.
            std::string label = boost::lexical_cast<std::string>(unit->num);
            auto lblPt = sPixelFromHex(hexFromAry(i));
            SDL_Rect lblBox;
            lblBox.x = lblPt.x;
            lblBox.y = lblPt.y + pHexSize - TTF_FontLineSkip(font_.get()) - 1;
            lblBox.w = pHexSize;
            lblBox.h = TTF_FontLineSkip(font_.get());
            sdlDrawText(font_, label, lblBox, WHITE, Justify::CENTER);
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
