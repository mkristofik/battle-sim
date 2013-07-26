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
#include <memory>
#include <vector>

class Anim
{
public:
    Anim();
    virtual ~Anim();

    virtual bool isDone() const;
    Uint32 getRunTime() const;
    void execute();

protected:
    Uint32 runTime_;

private:
    virtual void start();
    virtual void run(Uint32 elapsed) = 0;
    virtual void stop();

    bool done_;
    Uint32 startTime_;
}; 


class AnimMove : public Anim
{
public:
    AnimMove(const Unit &unit, Point hDest, Facing f);

private:
    void start() override;
    void run(Uint32 elapsed) override;
    void stop() override;

    const Unit &unit_;
    Point destHex_;
    bool faceLeft_;
};


class AnimAttack : public Anim
{
public:
    AnimAttack(const Unit &unit, Point hTgt);
    Uint32 getHitTime() const;

private:
    void start() override;
    void run(Uint32 elapsed) override;
    void stop() override;

    // Move unit toward target for half the runtime and move it back for the
    // other half.
    void setPosition(Uint32 elapsed);
    void setFrame(Uint32 elapsed);

    const Unit &unit_;
    Point hTarget_;
    bool faceLeft_;
};


class AnimDefend : public Anim
{
public:
    AnimDefend(const Unit &unit, Point hSrc, Uint32 hitsAt);

private:
    void run(Uint32 elapsed) override;
    void stop() override;

    const Unit &unit_;
    Point hAttacker_;
    bool faceLeft_;
    Uint32 hitTime_;
};


class AnimRanged : public Anim
{
public:
    AnimRanged(const Unit &unit, Point hTgt);
    Uint32 getShotTime() const;  // time when projectile is fired

private:
    void run(Uint32 elapsed) override;
    void stop() override;

    void setFrame(Uint32 elapsed);

    const Unit &unit_;
    Point hTarget_;
    bool faceLeft_;
};


class AnimProjectile : public Anim
{
public:
    AnimProjectile(SdlSurface img, Point hSrc, Point hTgt, Uint32 shotTime);
    Uint32 getFlightTime() const;

private:
    void run(Uint32 elapsed) override;
    void stop() override;

    int id_;
    Point hTarget_;
    Uint32 shotTime_;
    Uint32 flightTime_;
    static const Uint32 timePerHex_ = 150;
};


class AnimParallel : public Anim
{
public:
    AnimParallel();
    void add(std::unique_ptr<Anim> &&anim);

private:
    bool isDone() const override;
    void start() override;
    void run(Uint32 elapsed) override;
    void stop() override;

    std::vector<std::unique_ptr<Anim>> animList_;
};

#endif
