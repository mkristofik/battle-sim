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
#include "Anim.h"
#include "GameState.h"

namespace
{
    // Restore the unit to the center of its hex and return to the base image.
    void idle(const Unit &unit, bool faceLeft)
    {
        auto &entity = gs->getEntity(unit.entityId);
        entity.pOffset = {0, 0};
        entity.frame = -1;
        entity.z = ZOrder::CREATURE;

        if (faceLeft) {
            entity.img = unit.type->reverseImg[unit.team];
        }
        else {
            entity.img = unit.type->baseImg[unit.team];
        }
    }
}

Anim::Anim()
    : done_{false},
    startTime_{0}
{
}

Anim::~Anim()
{
}

bool Anim::isDone() const
{
    return done_;
}

void Anim::execute()
{
    if (startTime_ == 0) {
        startTime_ = SDL_GetTicks();
        start();
    }
    run();
    if (isDone()) {
        stop();
    }
}


AnimMove::AnimMove(const Unit &unit, Point hDest, Facing f)
    : unit_(unit),
    destHex_{std::move(hDest)},
    faceLeft_{f == Facing::LEFT}
{
    // Note: we can't use the unit's internal facing in here because that holds
    // the end state after all moves are done.  We need the facing for this
    // step only.
}

void AnimMove::start()
{
    auto &entity = gs->getEntity(unit_.entityId);
    if (faceLeft_) {
        if (!unit_.type->reverseImgMove.empty()) {
            entity.img = unit_.type->reverseImgMove[unit_.team];
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
        }
    }
    else {
        if (!unit_.type->imgMove.empty()) {
            entity.img = unit_.type->imgMove[unit_.team];
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
        }
    }

    entity.z = ZOrder::ANIMATING;
}

void AnimMove::run()
{
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= runtime_) {
        done_ = true;
        return;
    }

    auto &entity = gs->getEntity(unit_.entityId);
    auto frac = static_cast<double>(elapsed) / runtime_;
    auto dhex = pixelFromHex(destHex_) - pixelFromHex(entity.hex);
    entity.pOffset = dhex * frac;
}

void AnimMove::stop()
{
    auto &entity = gs->getEntity(unit_.entityId);
    entity.hex = destHex_;
    idle(unit_, faceLeft_);
}


AnimAttack::AnimAttack(const Unit &unit, Point hTgt)
    : unit_(unit),
    hTarget_{std::move(hTgt)},
    faceLeft_{unit.face == Facing::LEFT}
{
}

void AnimAttack::start()
{
    auto &entity = gs->getEntity(unit_.entityId);
    entity.z = ZOrder::ANIMATING;
}

void AnimAttack::run()
{
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= runtime_) {
        done_ = true;
        return;
    }
    setPosition(elapsed);
    setFrame(elapsed);
}

void AnimAttack::stop()
{
    idle(unit_, faceLeft_);
}

void AnimAttack::setPosition(Uint32 elapsed)
{
    auto &entity = gs->getEntity(unit_.entityId);
    auto pSrc = pixelFromHex(entity.hex);
    auto pTgt = pixelFromHex(hTarget_);
    auto halfway = (pTgt - pSrc) / 2;
    double halfTime = runtime_ / 2.0;

    if (elapsed < halfTime) {
        entity.pOffset = elapsed / halfTime * halfway;
    }
    else {
        entity.pOffset = (2 - elapsed / halfTime) * halfway;
    }
}

void AnimAttack::setFrame(Uint32 elapsed)
{
    auto &entity = gs->getEntity(unit_.entityId);
    const auto &frameList = unit_.type->attackFrames;
    int size = frameList.size();

    entity.frame = size - 1;
    for (int i = 0; i < size; ++i) {
        if (elapsed < frameList[i]) {
            entity.frame = i;
            break;
        }
    }

    if (faceLeft_) {
        if (!unit_.type->reverseAnimAttack.empty()) {
            entity.img = unit_.type->reverseAnimAttack[unit_.team];
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
            entity.frame = -1;
        }
    }
    else {
        if (!unit_.type->animAttack.empty()) {
            entity.img = unit_.type->animAttack[unit_.team];
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
            entity.frame = -1;
        }
    }
}
