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
#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include "SDL_stdinc.h"
#include "SDL_video.h"
#include "iterable_enum_class.h"
#include <iosfwd>
#include <limits>
#include <string>
#include <utility>
#include <vector>

const Sint16 Sint16_min = std::numeric_limits<Sint16>::min();
const Sint16 Sint16_max = std::numeric_limits<Sint16>::max();

struct Point
{
    int x;
    int y;

    Point() : x{Sint16_min}, y{Sint16_min} {}
    Point(int xPos, int yPos) : x{xPos}, y{yPos} {}
};

const Point hInvalid;
const Sint16 pHexSize = 72;

bool operator==(const Point &lhs, const Point &rhs);
bool operator!=(const Point &lhs, const Point &rhs);
Point operator+(const Point &lhs, const Point &rhs);
Point operator-(const Point &lhs, const Point &rhs);

template <typename T>
Point operator*(const Point &lhs, T rhs)
{
    return {static_cast<int>(lhs.x * rhs), static_cast<int>(lhs.y * rhs)};
}

template <typename T>
Point operator*(T lhs, const Point &rhs)
{
    return rhs * lhs;
}

template <typename T>
Point operator/(const Point &lhs, T rhs)
{
    return {lhs.x / rhs, lhs.y / rhs};
}

std::ostream & operator<<(std::ostream &ostr, const Point &rhs);

enum class Dir8 {None = -1, N, NE, E, SE, S, SW, W, NW};
enum class Dir {N, NE, SE, S, SW, NW, _last, _first = N};
ITERABLE_ENUM_CLASS(Dir);

// Distance between hexes, 1 step per tile.
Sint16 hexDist(const Point &h1, const Point &h2);

// Return the hex adjacent to the source hex in the given direction.  No
// bounds checking.
Point adjacent(const Point &hSrc, Dir d);

// Return the direction of travel between two hexes.
Dir direction(const Point &h1, const Point &h2);
Dir8 direction8(const Point &h1, const Point &h2);
double hexAngle_rad(const Point &h1, const Point &h2);

// Find which of the six triangles of a hex contains the given point.  (0,0) is
// the point where the hex is drawn (and thus is not itself within the hex).
// Assumes the given point is inside the hex.
Dir getSector(int hpx, int hpy);

// Given a list of hexes, return the index of the hex closest to the target.
int findClosest(const Point &hTarget, const std::vector<Point> &hexes);

// Return the pixel location where a hex is drawn.  You have to compute your
// own offsets if hex (0,0) isn't drawn in the upper-left corner of the window.
Point pixelFromHex(int hx, int hy);
Point pixelFromHex(Point hex);

// Return the hex that contains the given pixel.  Assumes hex (0,0) is drawn at
// pixel (0,0).
Point hexFromPixel(int px, int py);

#endif
