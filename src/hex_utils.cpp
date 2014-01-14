/*
    Copyright (C) 2013-2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "hex_utils.h"
#include <cassert>
#include <cmath>
#include <sstream>

bool operator==(const Point &lhs, const Point &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const Point &lhs, const Point &rhs)
{
    return !(lhs == rhs);
}

Point operator+(const Point &lhs, const Point &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

Point operator-(const Point &lhs, const Point &rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

std::ostream & operator<<(std::ostream &ostr, const Point &rhs)
{
    ostr << '(' << rhs.x << ',' << rhs.y << ')';
    return ostr;
}

// source: Battle for Wesnoth, distance_between() in map_location.cpp.
Sint16 hexDist(const Point &h1, const Point &h2)
{
    if (h1 == hInvalid || h2 == hInvalid) {
        return Sint16_max;
    }

    Sint16 dx = abs(h1.x - h2.x);
    Sint16 dy = abs(h1.y - h2.y);

    // Since the x-axis of the hex grid is staggered, we need to add a step in
    // certain cases.
    Sint16 vPenalty = 0;
    if ((h1.y < h2.y && h1.x % 2 == 0 && h2.x % 2 == 1) ||
        (h1.y > h2.y && h1.x % 2 == 1 && h2.x % 2 == 0)) {
        vPenalty = 1;
    }

    return std::max(dx, static_cast<Sint16>(dy + vPenalty + dx / 2));
}

Point adjacent(const Point &hSrc, Dir d)
{
    auto hx = hSrc.x;

    switch (d) {
        case Dir::N:
            return hSrc + Point{0, -1};
        case Dir::NE:
            if (hx % 2 == 0) {
                return hSrc + Point{1, -1};
            }
            else {
                return hSrc + Point{1, 0};
            }
        case Dir::SE:
            if (hx % 2 == 0) {
                return hSrc + Point{1, 0};
            }
            else {
                return hSrc + Point{1, 1};
            }
        case Dir::S:
            return hSrc + Point{0, 1};
        case Dir::SW:
            if (hx % 2 == 0) {
                return hSrc + Point{-1, 0};
            }
            else {
                return hSrc + Point{-1, 1};
            }
        case Dir::NW:
            if (hx % 2 == 0) {
                return hSrc + Point{-1, -1};
            }
            else {
                return hSrc + Point{-1, 0};
            }
        default:
            return hInvalid;
    }
}

Dir direction(const Point &h1, const Point &h2)
{
    auto angle = hexAngle_rad(h1, h2);

    // Hex angles are not perfect 60 degrees.  They're about 63 deg (1.1 rad).
    if (angle > 1.1 && angle <= 2.04) {
        return Dir::N;
    }
    else if (angle > 0 && angle <= 1.1) {
        return Dir::NE;
    }
    else if (angle > -1.1 && angle <= 0) {
        return Dir::SE;
    }
    else if (angle > -2.04 && angle <= 1.1) {
        return Dir::S;
    }
    else if (angle <= -2.04) {
        return Dir::SW;
    }
    else {
        return Dir::NW;
    }
}

double hexAngle_rad(const Point &h1, const Point &h2)
{
    // Our coordinate system has y-axis increasing as you go down the screen,
    // so we need to flip it to align with the cartesian plane.
    auto p1 = pixelFromHex(h1);
    auto p2 = pixelFromHex(h2);
    return atan2(p1.y - p2.y, p2.x - p1.x);
}

Dir getSector(int hpx, int hpy)
{
    if (hpy < pHexSize / 2) {  // top half
        if (hpy > 2 * hpx - pHexSize / 2) {  // greater than y = 2x-36
            return Dir::NW;
        }
        else if (hpy >= 3 * pHexSize / 2 - 2 * hpx) {  // greater than y = 108-2x
            return Dir::NE;
        }
        else {
            return Dir::N;
        }
    }
    else {  // bottom half
        if (hpy < 3 * pHexSize / 2 - 2 * hpx) {  // less than y = 108-2x
            return Dir::SW;
        }
        else if (hpy <= 2 * hpx - pHexSize / 2) {  // less than y = 2x-36
            return Dir::SE;
        }
        else {
            return Dir::S;
        }
    }
}

int findClosest(const Point &hTarget, const std::vector<Point> &hexes)
{
    int closest = -1;
    int size = static_cast<int>(hexes.size());
    Sint16 bestSoFar = Sint16_max;

    for (int i = 0; i < size; ++i) {
        Sint16 dist = hexDist(hTarget, hexes[i]);
        if (dist < bestSoFar) {
            closest = i;
            bestSoFar = dist;
        }
    }

    return closest;
}

Point pixelFromHex(int hx, int hy)
{
    int px = hx * pHexSize * 3 / 4;
    int py = (hy + 0.5 * abs(hx % 2)) * pHexSize;
    return {px, py};
}

Point pixelFromHex(Point hex)
{
    return pixelFromHex(hex.x, hex.y);
}

Point hexFromPixel(int px, int py)
{
    // tilingWidth
    // |   |
    //  _     _
    // / \_    tilingHeight
    // \_/ \  _
    //   \_/
    const Sint16 tilingWidth = pHexSize * 3 / 2;
    const Sint16 tilingHeight = pHexSize;

    // I'm not going to pretend to know why the rest of this works.
    Sint16 hx = px / tilingWidth * 2;
    Sint16 xMod = px % tilingWidth;
    Sint16 hy = py / tilingHeight;
    Sint16 yMod = py % tilingHeight;

    if (yMod < tilingHeight / 2) {
        if ((xMod * 2 + yMod) < (pHexSize / 2)) {
            --hx;
            --hy;
        }
        else if ((xMod * 2 - yMod) < (pHexSize * 3 / 2)) {
            // do nothing
        }
        else {
            ++hx;
            --hy;
        }
    }
    else {
        if ((xMod * 2 - (yMod - pHexSize / 2)) < 0) {
            --hx;
        }
        else if ((xMod * 2 + (yMod - pHexSize / 2)) < pHexSize * 2) {
            // do nothing
        }
        else {
            ++hx;
        }
    }

    return {hx, hy};
}
