/*
    Copyright (C) 2012-2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef ALGO_H
#define ALGO_H

#include <algorithm>
#include <iterator>
#include <memory>
#include <random>
#include <string>

template <class Container, class T>
bool contains(const Container &c, const T &elem)
{
    return find(std::begin(c), std::end(c), elem) != std::end(c);
}

template <class T, class U, class V>
T bound(const T &val, const U &minVal, const V &maxVal)
{
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
}

template <class T, class... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::minstd_rand & randomGenerator();

template <class Container>
typename Container::const_iterator randomElem(const Container &c)
{
    if (c.empty()) return std::end(c);

    std::uniform_int_distribution<int> elems(0, c.size() - 1);
    int index = elems(randomGenerator());
    auto iter = std::begin(c);
    return std::next(iter, index);
}

std::string to_upper(std::string str);
std::string to_lower(std::string str);

#endif
