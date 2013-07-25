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
        start();
    }
    else {
        run();
    }
    if (isDone()) {
        stop();
    }
}

void Anim::start()
{
    startTime_ = SDL_GetTicks();
}


AnimMove::AnimMove(const Unit &unit, Point hDest, Facing f)
    : Anim(),
    unit_(unit),
    destHex_{std::move(hDest)},
    faceLeft_{f == Facing::LEFT}
{
    // Note: we can't use the unit's internal facing in here because that holds
    // the end state after all moves are done.  We need the facing for this
    // step only.
}

void AnimMove::start()
{
    Anim::start();

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
    // TODO: can we refactor this out at all?
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= runTime_) {
        done_ = true;
        return;
    }

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
}

void AnimAttack::start()
{
    Anim::start();
    auto &entity = gs->getEntity(unit_.entityId);
    entity.z = ZOrder::ANIMATING;
}

void AnimAttack::run()
{
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= runTime) {
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
    double halfTime = runTime / 2.0;

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

AnimRanged::AnimRanged(const Unit &unit, Point hTgt)
    : Anim(),
    unit_(unit),
    hTarget_{std::move(hTgt)},
    faceLeft_{unit.face == Facing::LEFT}
{
}

void AnimRanged::run()
{
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= runTime) {
        done_ = true;
        return;
    }
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


AnimDefend::AnimDefend(const Unit &unit, Point hSrc, Uint32 hitsAt)
    : Anim(),
    unit_(unit),
    hAttacker_{std::move(hSrc)},
    faceLeft_{unit.face == Facing::LEFT},
    hitTime_{hitsAt},
    runTime_{hitTime_ + 250}
{
}

void AnimDefend::run()
{
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= runTime_) {
        done_ = true;
        return;
    }

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


AnimProjectile::AnimProjectile(SdlSurface img, Point hSrc, Point hTgt)
    : Anim(),
    id_{gs->addHiddenEntity(std::move(img), ZOrder::PROJECTILE)},
    hTarget_{std::move(hTgt)},
    shotTime_{AnimRanged::runTime / 2},
    flightTime_{hexDist(hSrc, hTarget_) * timePerHex}
{
    auto &entity = gs->getEntity(id_);
    entity.hex = std::move(hSrc);
    auto dir = direction(entity.hex, hTarget_);
    entity.frame = static_cast<int>(dir);
}

void AnimProjectile::run()
{
    auto elapsed = SDL_GetTicks() - startTime_;
    if (elapsed >= shotTime_ + flightTime_) {
        done_ = true;
        return;
    }
    else if (elapsed < shotTime_) {
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
    animList_{},
    stopped_{}
{
}

void AnimParallel::add(std::unique_ptr<Anim> &&anim)
{
    animList_.emplace_back(std::move(anim));
    stopped_.push_back(false);
}

bool AnimParallel::isDone() const
{
    return all_of(std::begin(animList_), std::end(animList_),
                  std::mem_fn(&Anim::isDone));
}

void AnimParallel::start()
{
    Anim::start();
    for (auto &anim : animList_) {
        anim->start();
    }
}

void AnimParallel::run()
{
    for (auto i = 0u; i < animList_.size(); ++i) {
        if (!animList_[i]->isDone()) {
            animList_[i]->run();
        }
        else {
            if (!stopped_[i]) {
                animList_[i]->stop();
                stopped_[i] = true;
            }
        }
    }
}

void AnimParallel::stop()
{
    for (auto i = 0u; i < animList_.size(); ++i) {
        if (!stopped_[i]) {
            animList_[i]->stop();
            stopped_[i] = true;
        }
    }
}
