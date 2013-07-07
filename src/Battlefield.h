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

#include "HexGrid.h"
#include "hex_utils.h"
#include "sdl_helper.h"

// Handle the drawing of the hexagonal battlefield and all of the drawable
// entities on the battlefield.

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
};

class Battlefield
{
public:
    Battlefield(SDL_Rect dispArea);

    bool isHexValid(int hx, int hy) const;
    bool isHexValid(const Point &hex) const;
    bool isHexValid(int aIndex) const;

    // Return the hex containing the given screen coordinates.
    Point hexFromPixel(int spx, int spy) const;
    int aryFromPixel(int spx, int spy) const;

    // Get a list of hexes adjacent to the given hex.
    std::vector<int> aryNeighbors(int aIndex) const;

    // Add a drawable entity to the battlefield.  Return its unique id number.
    int addEntity(Point hex, SdlSurface img, ZOrder z);
    int addHiddenEntity(SdlSurface img, ZOrder z);

    const Drawable & getEntity(int id) const;

    // Functions for hex highlights.
    void showMouseover(int spx, int spy);
    void showMouseover(const Point &hex);
    void hideMouseover();
    void selectEntity(int id);
    void deselectEntity();
    void setMoveTarget(const Point &hex);
    void clearMoveTarget();
    void setRangedTarget(const Point &hex);
    void clearRangedTarget();
    void showAttackArrow(int spx, int spy);
    void hideAttackArrow();
    void clearHighlights();

    void draw();

private:
    // Return the screen coordinates of a hex.
    Point sPixelFromHex(int hx, int hy) const;
    Point sPixelFromHex(const Point &hex) const;

    SDL_Rect displayArea_;
    std::vector<Drawable> entities_;
    std::vector<int> entityIds_;
    HexGrid grid_;

    // Entities for display features.
    int hexShadow_;
    int redHex_;
    int yellowHex_;
    int greenHex_;
    int attackSrc_;
    int attackTgt_;
};

#endif
