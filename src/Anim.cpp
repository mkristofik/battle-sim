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
#include "Effects.h"
#include "Traits.h"
#include "algo.h"

#include <algorithm>
#include <cassert>
#include <functional>

namespace
{
    // Return the appropriate base image for a unit given its facing.
    SdlSurface getBaseImage(const Unit &unit)
    {
        if (unit.face == Facing::LEFT) {
            return unit.type->reverseImg[unit.team];
        }
        return unit.type->baseImg[unit.team];
    }

    // Restore the unit to the center of its hex and return to the base image.
    void idle(Drawable &entity, const Unit &unit)
    {
        entity.frame = -1;
        entity.z = ZOrder::CREATURE;
        entity.img = getBaseImage(unit);
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
        auto &font = sdlGetFont(FontType::SMALL);
        label.img = sdlRenderText(font, size, getLabelColor(unit.team));
        label.alignBottomCenter();
    }
}

Battlefield *Anim::bf_ = nullptr;

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


AnimMove::AnimMove(Unit unit, const Point &hSrc, Point hDest)
    : Anim(),
    unit_{std::move(unit)},
    destHex_{std::move(hDest)}
{
    runTime_ = 300 * hexDist(hSrc, destHex_);
    // Note: we can't use the unit's internal facing in here because that holds
    // the end state after all moves are done.  We need the facing for this
    // step only.
}

void AnimMove::start()
{
    auto &entity = bf_->getEntity(unit_.entityId);
    if (unit_.face == Facing::LEFT) {
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
    idle(entity, unit_);

    auto &labelEntity = bf_->getEntity(unit_.labelId);
    labelEntity.hex = destHex_;
    labelEntity.visible = true;
}


AnimAttack::AnimAttack(Unit unit, Point hTgt)
    : Anim(),
    unit_{std::move(unit)},
    hTarget_{std::move(hTgt)}
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
    idle(bf_->getEntity(unit_.entityId), unit_);

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

    if (unit_.face == Facing::LEFT) {
        if (!unit_.type->reverseAnimAttack.empty() && entity.frame >= 0) {
            entity.img = unit_.type->reverseAnimAttack[unit_.team];
            entity.numFrames = unit_.type->attackFrames.size();
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
            entity.numFrames = 1;
        }
    }
    else {
        if (!unit_.type->animAttack.empty() && entity.frame >= 0) {
            entity.img = unit_.type->animAttack[unit_.team];
            entity.numFrames = unit_.type->attackFrames.size();
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
            entity.numFrames = 1;
        }
    }
}


AnimDefend::AnimDefend(Unit unit, Uint32 hitsAt)
    : Anim(),
    unit_{std::move(unit)},
    hitTime_{hitsAt}
{
    runTime_ = hitTime_ + 250;
}

void AnimDefend::run(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
    if (unit_.face == Facing::LEFT) {
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
    idle(bf_->getEntity(unit_.entityId), unit_);
    updateSize(bf_->getEntity(unit_.labelId), unit_, unit_.num);
}


AnimRanged::AnimRanged(Unit unit)
    : Anim(),
    unit_{std::move(unit)}
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
    idle(bf_->getEntity(unit_.entityId), unit_);
}

void AnimRanged::setFrame(Uint32 elapsed)
{
    auto &entity = bf_->getEntity(unit_.entityId);
    entity.frame = getFrame(unit_.type->rangedFrames, elapsed);

    if (unit_.face == Facing::LEFT) {
        if (!unit_.type->reverseAnimRanged.empty() && entity.frame >= 0) {
            entity.img = unit_.type->reverseAnimRanged[unit_.team];
            entity.numFrames = unit_.type->rangedFrames.size();
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
            entity.numFrames = 1;
        }
    }
    else {
        if (!unit_.type->animRanged.empty() && entity.frame >= 0) {
            entity.img = unit_.type->animRanged[unit_.team];
            entity.numFrames = unit_.type->rangedFrames.size();
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
            entity.numFrames = 1;
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


AnimDie::AnimDie(Unit unit, Uint32 hitsAt)
    : Anim(),
    unit_{std::move(unit)},
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

    if (unit_.face == Facing::LEFT) {
        if (!unit_.type->reverseAnimDie.empty() && entity.frame >= 0) {
            entity.img = unit_.type->reverseAnimDie[unit_.team];
            entity.numFrames = unit_.type->dieFrames.size();
        }
        else if (!unit_.type->reverseImgDefend.empty()) {
            entity.img = unit_.type->reverseImgDefend[unit_.team];
            entity.numFrames = 1;
        }
        else {
            entity.img = unit_.type->reverseImg[unit_.team];
            entity.numFrames = 1;
        }
    }
    else {
        if (!unit_.type->animDie.empty() && entity.frame >= 0) {
            entity.img = unit_.type->animDie[unit_.team];
            entity.numFrames = unit_.type->dieFrames.size();
        }
        else if (!unit_.type->imgDefend.empty()) {
            entity.img = unit_.type->imgDefend[unit_.team];
            entity.numFrames = 1;
        }
        else {
            entity.img = unit_.type->baseImg[unit_.team];
            entity.numFrames = 1;
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


AnimEffect::AnimEffect(Effect e, Unit target, Point hex, Uint32 startsAt)
    : effect_{std::move(e)},
    id_{-1},
    target_{std::move(target)},
    hex_{std::move(hex)},
    startTime_{startsAt}
{
    const auto &frames = effect_.getFrames();
    runTime_ = startTime_;
    if (!frames.empty()) {
        runTime_ += frames.back();
    }

    if (effect_.getAnim()) {
        id_ = bf_->addHiddenEntity(effect_.getAnim(), ZOrder::PROJECTILE);
        auto &entity = bf_->getEntity(id_);
        entity.hex = hex_;
        entity.frame = 0;
        entity.numFrames = frames.size();
        entity.alignBottomCenterAnim();
    }
}

void AnimEffect::run(Uint32 elapsed)
{
    if (elapsed < startTime_) {
        return;
    }

    if (effect_.type == EffectType::ENRAGED) {
        runEnraged(elapsed - startTime_);
    }
    else if (id_ >= 0) {
        runOther(elapsed - startTime_);
    }
}

void AnimEffect::runEnraged(Uint32 timeSinceStart)
{
    const auto &frames = effect_.getFrames();
    if (frames.size() < 2) return;

    auto fadeInTime = frames[0];
    auto fadeOutTime = frames[1];
    double frac = 0.0;
    if (timeSinceStart < fadeInTime) {
        frac = static_cast<double>(timeSinceStart) / fadeInTime;
    }
    else {
        frac = static_cast<double>(fadeOutTime - timeSinceStart) /
            (fadeOutTime - fadeInTime);
    }
    frac *= 0.6;  // don't get all the way to full red

    auto &entity = bf_->getEntity(target_.entityId);
    entity.img = sdlBlendColor(getBaseImage(target_), RED, frac);
}

void AnimEffect::runOther(Uint32 timeSinceStart)
{
    auto &entity = bf_->getEntity(id_);
    entity.visible = true;
    entity.frame = getFrame(effect_.getFrames(), timeSinceStart);
}

void AnimEffect::stop()
{
    if (effect_.type == EffectType::ENRAGED) {
        stopEnraged();
    }
    else if (id_ >= 0) {
        stopOther();
    }
}

void AnimEffect::stopEnraged()
{
    auto &entity = bf_->getEntity(target_.entityId);
    entity.img = getBaseImage(target_);
}

void AnimEffect::stopOther()
{
    auto &entity = bf_->getEntity(id_);
    entity.visible = false;
}
