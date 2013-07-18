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
#ifndef ANIM_H
#define ANIM_H

#include "Unit.h"
#include "hex_utils.h"
#include "sdl_helper.h"

class Anim
{
public:
    Anim();
    virtual ~Anim();

    bool isDone() const;
    void execute();

protected:
    bool done_;
    Uint32 startTime_;

private:
    virtual void start() = 0;
    virtual void run() = 0;
    virtual void stop() = 0;
}; 

class AnimMove : public Anim
{
public:
    AnimMove(const Unit &unit, Point hDest, Facing f);

private:
    void start() override;
    void run() override;
    void stop() override;

    const Unit &unit_;
    Point destHex_;
    bool faceLeft_;
    static const Uint32 runTime_ = 300;
};

#endif
