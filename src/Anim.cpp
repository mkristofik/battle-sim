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
#include "Anim.h"
#include "Battlefield.h"
#include "Traits.h"

#include <algorithm>
#include <cassert>
#include <functional>

namespace
{
    // Restore the unit to the center of its hex and return to the base image.
    void idle(Drawable &entity, const Unit &unit, bool faceLeft)
    {
        entity.frame = -1;
        entity.z = ZOrder::CREATURE;

        if (faceLeft) {
            entity.img = unit.type->reverseImg[unit.team];
        }
        else {
            entity.img = unit.type->baseImg[unit.team];
        }
        entity.alignCenter();
    }

    // Compute the frame index to use.
    int getFrame(const FrameList &frames, Uint32 elapsed)
    {
        if (frames.empty()) return -1;

        int size = frames.size();
        for (int i = 0; i < size; ++i) {
            if (elapsed < frames[i]) {
                return i;
            }
        }

        return size - 1;
    }

    void updateSize(Drawable &label, const Unit &unit, int size)
    {
        assert(label.font);
        label.img = sdlPreRender(label.font, size, getLabelColor(unit.team));
        label.alignBottomCenter();
    }
}

Battlefield *Anim::bf_ = nullptr;
SdlSurface AnimRegenerate::overlay_;
FrameList AnimRegenerate::timings_;

void Anim::setBattlefield(Battlefield &b)
{
    bf_ = &b;
}

Anim::Anim()
    : runTime_{0},
    done_{false},
    startTime_{0}
{
    assert(bf_);
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


AnimMove::AnimMove(const Unit &unit, const Point &hSrc, Point hDest, Facing f)
    : Anim(),
    unit_(unit),
    destHex_{std::move(hDest)},
    faceLeft_{f == Facing::LEFT}
{
    runTime_ = 300 * hexDist(hSrc, destHex_);
    // Note: we can't use the unit's internal facing in here because that holds
    // the end state after all moves are done.  We need the facing for this
    // step only.
}

void AnimMove::start()
{
    auto &entity = bf_->getEntity(unit_.entityId);
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

    auto &labelEntity = bf_->getEntity(unit_.labelId);
    labelEntity.visible = false;
}

void AnimMove::run(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
    auto frac = static_cast<double>(elapsed) / runTime_;
    auto dhex = pixelFromHex(destHex_) - pixelFromHex(entity.hex);
    entity.pOffset = dhex * frac;
}

void AnimMove::stop()
{
    auto &entity = bf_->getEntity(unit_.entityId);
    entity.hex = destHex_;
    idle(entity, unit_, faceLeft_);

    auto &labelEntity = bf_->getEntity(unit_.labelId);
    labelEntity.hex = destHex_;
    labelEntity.visible = true;
}


AnimAttack::AnimAttack(Unit unit, Point hTgt)
    : Anim(),
    unit_{std::move(unit)},
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
    auto &entity = bf_->getEntity(unit_.entityId);
    entity.z = ZOrder::ANIMATING;
}

void AnimAttack::run(Uint32 elapsed)
{
    setPosition(elapsed);
    setFrame(elapsed);
}

void AnimAttack::stop()
{
    idle(bf_->getEntity(unit_.entityId), unit_, faceLeft_);

    // Allow for attacks that can grow the number of units.
    updateSize(bf_->getEntity(unit_.labelId), unit_, unit_.num);
}

void AnimAttack::setPosition(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
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
    auto &entity = bf_->getEntity(unit_.entityId);
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


AnimDefend::AnimDefend(Unit unit, Point hSrc, Uint32 hitsAt)
    : Anim(),
    unit_{std::move(unit)},
    hAttacker_{std::move(hSrc)},
    faceLeft_{unit.face == Facing::LEFT},
    hitTime_{hitsAt}
{
    runTime_ = hitTime_ + 250;
}

void AnimDefend::run(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
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
    idle(bf_->getEntity(unit_.entityId), unit_, faceLeft_);
    updateSize(bf_->getEntity(unit_.labelId), unit_, unit_.num);
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
    idle(bf_->getEntity(unit_.entityId), unit_, faceLeft_);
}

void AnimRanged::setFrame(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
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
    id_{bf_->addHiddenEntity(std::move(img), ZOrder::PROJECTILE)},
    pStart_{},
    hTarget_{std::move(hTgt)},
    shotTime_{shotTime},
    flightTime_{hexDist(hSrc, hTarget_) * timePerHex_}
{
    runTime_ = shotTime_ + flightTime_;

    auto &entity = bf_->getEntity(id_);
    entity.hex = std::move(hSrc);
    entity.img = sdlRotate(entity.img, hexAngle_rad(entity.hex, hTarget_));
    entity.alignCenter();
    pStart_ = entity.pOffset;
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

    auto &entity = bf_->getEntity(id_);
    entity.visible = true;

    auto pSrc = pixelFromHex(entity.hex);
    auto pTgt = pixelFromHex(hTarget_);
    auto frac = static_cast<double>(elapsed - shotTime_) / flightTime_;
    entity.pOffset = pStart_ + (pTgt - pSrc) * frac * 0.9;
    // Projectiles are drawn with their trailing edge at the center of the hex.
    // Instead of doing all the work to figure out where their leading edge
    // needs to hit, we just shorten the flight distance by a little bit.
}

void AnimProjectile::stop()
{
    auto &entity = bf_->getEntity(id_);
    entity.visible = false;
    entity.img.reset();
}


AnimDie::AnimDie(const Unit &unit, Uint32 hitsAt)
    : Anim(),
    unit_(unit),
    faceLeft_{unit_.face == Facing::LEFT},
    hitTime_{hitsAt},
    fadeTime_{hitsAt}
{
    runTime_ = hitTime_ + fadeLength_;
    if (!unit_.type->dieFrames.empty()) {
        runTime_ += unit_.type->dieFrames.back();
        fadeTime_ += unit_.type->dieFrames.back();
    }
}

void AnimDie::run(Uint32 elapsed)
{
    if (elapsed < hitTime_) {
        return;
    }

    auto &label = bf_->getEntity(unit_.labelId);
    label.visible = false;
    setFrame(elapsed);

    // Fade out the unit until it disappears.
    if (elapsed > fadeTime_) {
        auto &entity = bf_->getEntity(unit_.entityId);
        auto frac = static_cast<double>(fadeLength_ - (elapsed - fadeTime_)) /
            fadeLength_;
        entity.img = sdlSetAlpha(entity.img, frac);
    }
}

void AnimDie::stop()
{
    auto &entity = bf_->getEntity(unit_.entityId);
    entity.visible = false;
}

void AnimDie::setFrame(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
    entity.frame = getFrame(unit_.type->dieFrames, elapsed - hitTime_);

    if (faceLeft_) {
        if (!unit_.type->reverseAnimDie.empty() && entity.frame >= 0) {
            entity.img = unit_.type->reverseAnimDie[unit_.team];
        }
        else if (!unit_.type->reverseImgDefend.empty()) {
            entity.img = unit_.type->reverseImgDefend[unit_.team];
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
        }
    }
    else {
        if (!unit_.type->animDie.empty() && entity.frame >= 0) {
            entity.img = unit_.type->animDie[unit_.team];
        }
        else if (!unit_.type->imgDefend.empty()) {
            entity.img = unit_.type->imgDefend[unit_.team];
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
        }
    }
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


AnimRegenerate::AnimRegenerate(Point hex)
    : id_{-1},
    hex_{std::move(hex)}
{
    if (!overlay_) {
        overlay_ = sdlLoadImage("regeneration.png");
        timings_ = {75, 150, 225, 300, 375, 450, 525, 600};
    }
    runTime_ = timings_.back();
}

void AnimRegenerate::start()
{
    id_ = bf_->addEntity(hex_, overlay_, ZOrder::ANIMATING);
    auto &entity = bf_->getEntity(id_);
    entity.frame = 0;
}

void AnimRegenerate::run(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(id_);
    entity.frame = getFrame(timings_, elapsed);
}

void AnimRegenerate::stop()
{
    auto &entity = bf_->getEntity(id_);
    entity.visible = false;
}
