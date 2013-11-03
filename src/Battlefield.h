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

#include "Drawable.h"
#include "hex_utils.h"
#include "sdl_helper.h"

class Action;
class HexGrid;

// Handle the drawing of the hexagonal battlefield and all of the drawable
// entities on the battlefield.

class Battlefield
{
public:
    Battlefield(SDL_Rect dispArea, const HexGrid &bfGrid);

    // Add a drawable entity to the battlefield.  Return its unique id number.
    int addEntity(Point hex, SdlSurface img, ZOrder z);
    int addHiddenEntity(SdlSurface img, ZOrder z);

    // References might get invalidated at any time.  Recommend calling this
    // instead of holding on to entity object refs for too long.
    Drawable & getEntity(int id);
    const Drawable & getEntity(int id) const;

    // Return the hex containing the given screen coordinates.
    Point hexFromPixel(int spx, int spy) const;
    int aryFromPixel(int spx, int spy) const;

    // Return the screen coordinates of a hex.
    Point sPixelFromHex(int hx, int hy) const;
    Point sPixelFromHex(const Point &hex) const;

    void handleMouseMotion(const SDL_MouseMotionEvent &event,
                           const Action &action);

    // Functions for hex highlights.
    void showMouseover(int spx, int spy);
    void showMouseover(const Point &hex);
    void showMouseover(int aIndex);
    void hideMouseover();
    void selectHex(int aIndex);
    void deselectHex();
    void setMoveTarget(const Point &hex);
    void setMoveTarget(int aIndex);
    void clearMoveTarget();
    void setRangedTarget(const Point &hex);
    void setRangedTarget(int aIndex);
    void clearRangedTarget();
    void showAttackArrow(int aSrc, int aTgt);
    void hideAttackArrow();
    void clearHighlights();

    void draw() const;

private:
    SDL_Rect displayArea_;
    const HexGrid &grid_;
    std::vector<Drawable> entities_;

    // Entities for display features.
    int hexShadow_;
    int redHex_;
    int yellowHex_;
    int greenHex_;
    int attackSrc_;
    int attackTgt_;
};

#endif
