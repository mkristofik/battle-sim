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
#include "Effects.h"

#include "Action.h"
#include "GameState.h"
#include "Unit.h"
#include "UnitType.h"
#include "algo.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <unordered_map>

namespace
{
    std::unordered_map<int, std::unique_ptr<EffectData>> cache;
    std::unordered_map<std::string, EffectType> allEffects;
    std::unordered_map<std::string, Duration> allDurations;

    const EffectData * getData(EffectType type)
    {
        auto iter = cache.find(static_cast<int>(type));
        assert(iter != cache.end());
        return iter->second.get();
    }
}


EffectData::EffectData(EffectType t, const rapidjson::Value &json)
    : type{t},
    anim{},
    animFrames{},
    dur{Duration::INSTANT},
    text{}
{
    if (json.HasMember("anim")) {
        anim = sdlLoadImage(json["anim"].GetString());
    }
    if (json.HasMember("anim-frames")) {
        animFrames = jsonListUnsigned(json["anim-frames"]);
    }
    if (json.HasMember("duration")) {
        const auto &durType = json["duration"].GetString();
        auto iter = allDurations.find(to_upper(durType));
        if (iter != std::end(allDurations)) {
            dur = iter->second;
        }
        else {
            std::cerr << "Warning: unrecognized duration type for effect " <<
                " type " << static_cast<int>(type) << '\n';
        }
    }
    if (json.HasMember("text")) {
        text = json["text"].GetString();
    }
}


EffectBound::EffectBound(EffectType type, const rapidjson::Value &json)
    : EffectData{type, json}
{
}

void EffectBound::apply(GameState &gs, Effect &effect, Unit &unit) const
{
    const int &attId = effect.data1;
    const int &attHex = effect.data2;

    const auto &attacker = gs.getUnit(attId);
    if (!attacker.isAlive() || attacker.aHex != attHex) {
        effect.dispose();
        return;
    }
}

Effect EffectBound::create(const GameState &gs, const Action &action) const
{
    const auto &att = gs.getUnit(action.attacker);

    Effect e;
    e.type = EffectType::BOUND;
    e.roundsLeft = 999;
    e.data1 = att.entityId;
    e.data2 = att.aHex;
    return e;
}


EffectSimple::EffectSimple(EffectType type, const rapidjson::Value &json)
    : EffectData{type, json}
{
}

void EffectSimple::apply(GameState &gs, Effect &effect, Unit &unit) const
{
}

Effect EffectSimple::create(const GameState &gs, const Action &action) const
{
    Effect e;
    e.type = type;
    if (dur == Duration::STANDARD) {
        e.roundsLeft = 4;
    }
    return e;
}


Effect::Effect()
    : type{EffectType::NONE},
    roundsLeft{0},
    data1{0},
    data2{0}
{
}

Effect::Effect(const GameState &gs, const Action &action, EffectType t)
    : Effect{}
{
    assert(t != EffectType::NONE);
    *this = getData(t)->create(gs, action);
}

const SdlSurface & Effect::getAnim() const
{
    assert(type != EffectType::NONE);
    return getData(type)->anim;
}

const FrameList & Effect::getFrames() const
{
    assert(type != EffectType::NONE);
    return getData(type)->animFrames;
}

const std::string & Effect::getText() const
{
    assert(type != EffectType::NONE);
    return getData(type)->text;
}

bool Effect::isDone() const
{
    if (type == EffectType::NONE) return false;
    return roundsLeft <= 0;
}

void Effect::apply(GameState &gs, Unit &unit)
{
    assert(type != EffectType::NONE);
    getData(type)->apply(gs, *this, unit);
}

void Effect::dispose()
{
    roundsLeft = 0;
}


bool initEffectCache(const char *filename)
{
#define X(str) allEffects.emplace(#str, EffectType::str);
    EFFECT_TYPES
#undef X
#define X(str) allDurations.emplace(#str, Duration::str);
    DURATION_TYPES
#undef X

    rapidjson::Document doc;
    if (!jsonParse(filename, doc)) return false;

    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "effects: skipping id '" << i->name.GetString() <<
                "'\n";
            continue;
        }

        auto effectIter = allEffects.find(to_upper(i->name.GetString()));
        if (effectIter == std::end(allEffects)) {
            std::cerr << "effects: unknown type '" << i->name.GetString() <<
                "'\n";
            continue;
        }

        auto type = effectIter->second;
        if (type == EffectType::BOUND) {
            cache.emplace(static_cast<int>(type),
                          make_unique<EffectBound>(type, i->value));
        }
        else {
            cache.emplace(static_cast<int>(type),
                          make_unique<EffectSimple>(type, i->value));
        }
    }

    return true;
}

EffectType effectFromStr(const std::string &str)
{
    auto i = allEffects.find(to_upper(str));
    if (i == std::end(allEffects)) {
        return EffectType::NONE;
    }
    return i->second;
}
