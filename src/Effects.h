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
#ifndef EFFECTS_H
#define EFFECTS_H

#include "sdl_helper.h"

#include "rapidjson/document.h"
#include <string>

struct Action;
class GameState;
struct Unit;

#define DURATION_TYPES \
    X(INSTANT) \
    X(UNTIL_ATT_MOVES)

#define EFFECT_TYPES \
    X(NONE) \
    X(BOUND) \
    X(HEAL) \
    X(LIGHTNING)

#define X(str) str,
enum class Duration {DURATION_TYPES};
enum class EffectType {EFFECT_TYPES};
#undef X

// Ideas
// Bless/Blessed
// - unit always does max damage
// - cancels: curse
// - needs: duration
//
// Curse/Cursed
// - unit always does min damage
// - cancels: bless
// - needs: duration
//
// Hurricane Winds/Grounded
// - all creatures lose flying
// - needs: duration
//
// I don't think I can configure effects completely in data without writing
// some kind of minilanguage.
//
// Traits are conceptually just permanent effects.  But!  They apply to the
// unit type, not an individual unit.
struct Effect;

struct EffectData
{
    EffectType type;
    SdlSurface anim;
    FrameList animFrames;
    Duration dur;
    std::string text;

    EffectData();  // TODO: remove this
    EffectData(EffectType t, const rapidjson::Value &json);

    virtual void apply(GameState &gs, Effect &effect, Unit &unit) const=0;
    virtual Effect create(const GameState &gs, const Action &action) const=0;
};

// Defender is stuck in current hex until attacker moves or is killed.
struct EffectBound : public EffectData
{
    EffectBound();
    EffectBound(EffectType type, const rapidjson::Value &json);
    void apply(GameState &gs, Effect &effect, Unit &unit) const override;
    Effect create(const GameState &gs, const Action &action) const override;
};

struct EffectSimple : public EffectData
{
    EffectSimple(EffectType type, const rapidjson::Value &json);
    void apply(GameState &gs, Effect &effect, Unit &unit) const override;
    Effect create(const GameState &gs, const Action &action) const override;
};


// Generic instance of an effect type to be applied to a unit.  Try to keep
// this as small as possible.
struct Effect
{
    EffectType type;
    int roundsLeft;
    int data1;
    int data2;

    Effect();
    Effect(const GameState &gs, const Action &action, EffectType t);

    const SdlSurface & getAnim() const;
    const FrameList & getFrames() const;
    const std::string & getText() const;
    bool isDone() const;  // can we remove this effect from the unit?

    void apply(GameState &gs, Unit &unit);
    void dispose();  // postcondition: isDone() == true
};

// Call this after SDL initialized but before the game starts.
void initEffectCache(const rapidjson::Document &doc);

#endif
