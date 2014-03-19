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
#include "Spells.h"

#include "algo.h"
#include <memory>
#include <unordered_map>

namespace
{
    std::unordered_map<int, std::unique_ptr<Spell>> cache;
    std::unordered_map<std::string, SpellType> allSpells;
}

Spell::Spell(SpellType t)
    : name{},
    damage{0},
    type{t},
    target{SpellTarget::ENEMY},
    effect{EffectType::NONE}
{
}

void initSpellCache()
{
#define X(str) allSpells.emplace(#str, SpellType::str);
    SPELL_TYPES
#undef X

    // TODO: make this configurable
    auto lightning = make_unique<Spell>(SpellType::LIGHTNING);
    lightning->name = "Lightning Bolt";
    lightning->target = SpellTarget::ENEMY;
    lightning->damage = 10;
    lightning->effect = EffectType::LIGHTNING;
    cache.emplace(static_cast<int>(SpellType::LIGHTNING), std::move(lightning));
}

const Spell * getSpell(SpellType type)
{
    auto iter = cache.find(static_cast<int>(type));
    if (iter == std::end(cache)) return nullptr;
    return iter->second.get();
}

const Spell * getSpell(const std::string &type)
{
    auto iter = allSpells.find(type);
    if (iter == std::end(allSpells)) return nullptr;
    return getSpell(iter->second);
}
