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

#include <algorithm>
#include <functional>

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

    // Compute the frame index to use.
    int getFrame(const FrameList &frames, Uint32 elapsed)
    {
        int size = frames.size();
        for (int i = 0; i < size; ++i) {
            if (elapsed < frames[i]) {
                return i;
            }
        }

        return -1;
    }
}

Anim::Anim()
    : runTime_{0},
    done_{false},
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

Uint32 Anim::getRunTime() const
{
    return runTime_;
}

void Anim::execute()
{
    if (startTime_ == 0) {
        startTime_ = SDL_GetTicks();
        start();
    }
    else {
        auto elapsed = SDL_GetTicks() - startTime_;
        if (elapsed < runTime_) {
            run(elapsed);
        }
        else if (!isDone()) {
            stop();
            done_ = true;
        }
    }
}

void Anim::start()
{
}

void Anim::stop()
{
}


AnimMove::AnimMove(const Unit &unit, Point hDest, Facing f)
    : Anim(),
    unit_(unit),
    destHex_{std::move(hDest)},
    faceLeft_{f == Facing::LEFT}
{
    runTime_ = 300;
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

void AnimMove::run(Uint32 elapsed)
{
    auto &entity = gs->getEntity(unit_.entityId);
    auto frac = static_cast<double>(elapsed) / runTime_;
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
    : Anim(),
    unit_(unit),
    hTarget_{std::move(hTgt)},
    faceLeft_{unit.face == Facing::LEFT}
{
    runTime_ = 600;
}

Uint32 AnimAttack::getHitTime() const
{
    return runTime_ / 2;
}

void AnimAttack::start()
{
    auto &entity = gs->getEntity(unit_.entityId);
    entity.z = ZOrder::ANIMATING;
}

void AnimAttack::run(Uint32 elapsed)
{
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
    double halfTime = runTime_ / 2.0;

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
    entity.frame = getFrame(unit_.type->attackFrames, elapsed);

    if (faceLeft_) {
        if (!unit_.type->reverseAnimAttack.empty() && entity.frame >= 0) {
            entity.img = unit_.type->reverseAnimAttack[unit_.team];
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
        }
    }
    else {
        if (!unit_.type->animAttack.empty() && entity.frame >= 0) {
            entity.img = unit_.type->animAttack[unit_.team];
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
        }
    }
}


AnimDefend::AnimDefend(const Unit &unit, Point hSrc, Uint32 hitsAt)
    : Anim(),
    unit_(unit),
    hAttacker_{std::move(hSrc)},
    faceLeft_{unit.face == Facing::LEFT},
    hitTime_{hitsAt}
{
    runTime_ = hitTime_ + 250;
}

void AnimDefend::run(Uint32 elapsed)
{
    auto &entity = gs->getEntity(unit_.entityId);
    if (faceLeft_) {
        if (elapsed >= hitTime_ && !unit_.type->reverseImgDefend.empty()) {
            entity.img = unit_.type->reverseImgDefend[unit_.team];
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
        }
    }
    else {
        if (elapsed >= hitTime_ && !unit_.type->imgDefend.empty()) {
            entity.img = unit_.type->imgDefend[unit_.team];
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
        }
    }
}

void AnimDefend::stop()
{
    idle(unit_, faceLeft_);
}


AnimRanged::AnimRanged(const Unit &unit, Point hTgt)
    : Anim(),
    unit_(unit),
    hTarget_{std::move(hTgt)},
    faceLeft_{unit.face == Facing::LEFT}
{
    runTime_ = 600;
}

Uint32 AnimRanged::getShotTime() const
{
    return runTime_ / 2;
}

void AnimRanged::run(Uint32 elapsed)
{
    setFrame(elapsed);
}

void AnimRanged::stop()
{
    idle(unit_, faceLeft_);
}

void AnimRanged::setFrame(Uint32 elapsed)
{
    auto &entity = gs->getEntity(unit_.entityId);
    entity.frame = getFrame(unit_.type->rangedFrames, elapsed);

    if (faceLeft_) {
        if (!unit_.type->reverseAnimRanged.empty() && entity.frame >= 0) {
            entity.img = unit_.type->reverseAnimRanged[unit_.team];
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
        }
    }
    else {
        if (!unit_.type->animRanged.empty() && entity.frame >= 0) {
            entity.img = unit_.type->animRanged[unit_.team];
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
        }
    }
}


AnimProjectile::AnimProjectile(SdlSurface img, Point hSrc, Point hTgt,
                               Uint32 shotTime)
    : Anim(),
    id_{gs->addHiddenEntity(std::move(img), ZOrder::PROJECTILE)},
    hTarget_{std::move(hTgt)},
    shotTime_{shotTime},
    flightTime_{hexDist(hSrc, hTarget_) * timePerHex_}
{
    runTime_ = shotTime_ + flightTime_;

    auto &entity = gs->getEntity(id_);
    entity.hex = std::move(hSrc);

    // Support projectile images that can rotate to face the target.
    if (entity.img->w == pHexSize * 6) {
        auto dir = direction(entity.hex, hTarget_);
        entity.frame = static_cast<int>(dir);
    }
    else if (entity.img->w == pHexSize * 8) {
        auto dir = direction8(entity.hex, hTarget_);
        entity.frame = static_cast<int>(dir);
    }
}

Uint32 AnimProjectile::getFlightTime() const
{
    return flightTime_;
}

void AnimProjectile::run(Uint32 elapsed)
{
    if (elapsed < shotTime_) {
        return;
    }

    auto &entity = gs->getEntity(id_);
    entity.visible = true;

    auto pSrc = pixelFromHex(entity.hex);
    auto pTgt = pixelFromHex(hTarget_);
    auto frac = static_cast<double>(elapsed - shotTime_) / flightTime_;
    entity.pOffset = (pTgt - pSrc) * frac;
}

void AnimProjectile::stop()
{
    auto &entity = gs->getEntity(id_);
    entity.visible = false;
    entity.img.reset();
}


AnimParallel::AnimParallel()
    : Anim(),
    animList_{}
{
}

void AnimParallel::add(std::unique_ptr<Anim> &&anim)
{
    animList_.emplace_back(std::move(anim));
}

bool AnimParallel::isDone() const
{
    return all_of(std::begin(animList_), std::end(animList_),
                  std::mem_fn(&Anim::isDone));
}

void AnimParallel::start()
{
    Uint32 maxTime = 0;
    for (const auto &anim : animList_) {
        maxTime = std::max(maxTime, anim->getRunTime());
    }
    runTime_ = maxTime;

    for (auto &anim : animList_) {
        anim->execute();
    }
}

void AnimParallel::run(Uint32 elapsed)
{
    for (auto &anim : animList_) {
        anim->execute();
    }
}

void AnimParallel::stop()
{
    for (auto &anim : animList_) {
        anim->execute();
    }
}
