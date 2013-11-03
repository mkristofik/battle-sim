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
#include "HexGrid.h"
#include "Unit.h"

#include "boost/lexical_cast.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>

Battlefield::Battlefield(SDL_Rect dispArea, const HexGrid &bfGrid)
    : displayArea_(std::move(dispArea)),
    grid_(bfGrid),
    entities_{},
    hexShadow_{addHiddenEntity(sdlLoadImage("hex-shadow.png"),
                               ZOrder::SHADOW)},
    redHex_{addHiddenEntity(sdlLoadImage("hex-red.png"),
                            ZOrder::HIGHLIGHT)},
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
    auto gridLines = sdlLoadImage("hex-grid.png");

    // Create the background terrain and hex grid.  They can be treated as
    // drawable entities like everything else.
    for (int hx = -1; hx <= grid_.width(); ++hx) {
        for (int hy = -1; hy <= grid_.height(); ++hy) {
            Point hex = {hx, hy};
            addEntity(hex, tile, ZOrder::TERRAIN);
            if (!grid_.offGrid(hex)) {
                addEntity(hex, gridLines, ZOrder::GRID);
            }
        }
    }
}

int Battlefield::addEntity(Point hex, SdlSurface img, ZOrder z)
{
    int id = entities_.size();
    entities_.emplace_back(std::move(hex), std::move(img), std::move(z));

    return id;
}

int Battlefield::addHiddenEntity(SdlSurface img, ZOrder z)
{
    auto id = addEntity(hInvalid, std::move(img), std::move(z));
    entities_[id].visible = false;
    return id;
}

Drawable & Battlefield::getEntity(int id)
{
    assert(id >= 0 && id < static_cast<int>(entities_.size()));
    return entities_[id];
}

const Drawable & Battlefield::getEntity(int id) const
{
    return const_cast<Battlefield *>(this)->getEntity(id);
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

void Battlefield::handleMouseMotion(const SDL_MouseMotionEvent &event,
                                    const Action &action)
{
    clearHighlights();
    if (action.type == ActionType::ATTACK) {
        auto aMoveTo = action.path.back();
        showMouseover(aMoveTo);
        showAttackArrow(aMoveTo, action.defender->aHex);
    }
    else if (action.type == ActionType::RANGED) {
        auto aTgt = action.defender->aHex;
        showMouseover(aTgt);
        setRangedTarget(aTgt);
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
    if (grid_.offGrid(hex)) {
        hideMouseover();
        return;
    }

    auto &shadow = getEntity(hexShadow_);
    shadow.hex = hex;
    shadow.visible = true;
}

void Battlefield::showMouseover(int aIndex)
{
    showMouseover(grid_.hexFromAry(aIndex));
}

void Battlefield::hideMouseover()
{
    getEntity(hexShadow_).visible = false;
}

void Battlefield::selectHex(int aIndex)
{
    auto &selected = getEntity(yellowHex_);
    selected.hex = grid_.hexFromAry(aIndex);
    selected.visible = true;
}

void Battlefield::deselectHex()
{
    getEntity(yellowHex_).visible = false;
}

void Battlefield::setMoveTarget(const Point &hex)
{
    if (grid_.offGrid(hex)) {
        clearMoveTarget();
        return;
    }

    auto &target = getEntity(greenHex_);
    target.hex = hex;
    target.visible = true;
}

void Battlefield::setMoveTarget(int aIndex)
{
    setMoveTarget(grid_.hexFromAry(aIndex));
}

void Battlefield::clearMoveTarget()
{
    getEntity(greenHex_).visible = false;
}

void Battlefield::setRangedTarget(const Point &hex)
{
    if (grid_.offGrid(hex)) {
        clearRangedTarget();
        return;
    }

    auto &target = getEntity(redHex_);
    target.hex = hex;
    target.visible = true;
}

void Battlefield::setRangedTarget(int aIndex)
{
    setRangedTarget(grid_.hexFromAry(aIndex));
}

void Battlefield::clearRangedTarget()
{
    getEntity(redHex_).visible = false;
}

void Battlefield::showAttackArrow(int aSrc, int aTgt)
{
    auto hSrc = grid_.hexFromAry(aSrc);
    auto hTgt = grid_.hexFromAry(aTgt);
    auto &source = getEntity(attackSrc_);
    auto &target = getEntity(attackTgt_);

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
    getEntity(attackSrc_).visible = false;
    getEntity(attackTgt_).visible = false;
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
    std::vector<int> entityIds(entities_.size());
    iota(std::begin(entityIds), std::end(entityIds), 0);

    // Arrange entities logically by z-order.
    sort(std::begin(entityIds), std::end(entityIds), [&] (int a, int b)
         { return getEntity(a).z < getEntity(b).z; });

    sdlClear(displayArea_);
    SdlSetClipRect(displayArea_, [&] {
        for (auto id : entityIds) {
            const auto &e = getEntity(id);
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
