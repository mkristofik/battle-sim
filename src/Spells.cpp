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
    std::unordered_map<std::string, SpellTarget> allTargets;
}

Spell::Spell(SpellType t, const rapidjson::Value &json)
    : name{},
    damage{0},
    type{t},
    target{SpellTarget::ENEMY},
    effect{EffectType::NONE},
    cost{0}
{
    if (json.HasMember("name")) {
        name = json["name"].GetString();
    }
    if (json.HasMember("target")) {
        auto i = allTargets.find(to_upper(json["target"].GetString()));
        if (i != std::end(allTargets)) {
            target = i->second;
        }
        else {
            std::cerr << "Warning: unrecognized target type for spell " <<
                " type " << static_cast<int>(type) << '\n';
        }
    }
    if (json.HasMember("damage")) {
        damage = json["damage"].GetInt();
    }
    if (json.HasMember("effect")) {
        auto effType = json["effect"].GetString();
        effect = effectFromStr(effType);
        if (effect == EffectType::NONE) {
            std::cerr << "Warning: unrecognized effect type for spell " <<
                " type " << static_cast<int>(type) << '\n';
        }
    }
    if (json.HasMember("cost")) {
        cost = json["cost"].GetInt();
    }
}

bool initSpellCache(const char *filename)
{
#define X(str) allSpells.emplace(#str, SpellType::str);
    SPELL_TYPES
#undef X
#define X(str) allTargets.emplace(#str, SpellTarget::str);
    SPELL_TARGETS
#undef X

    rapidjson::Document doc;
    if (!jsonParse(filename, doc)) return false;

    for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
        if (!i->value.IsObject()) {
            std::cerr << "spells: skipping id '" << i->name.GetString() <<
                "'\n";
            continue;
        }

        auto spellIter = allSpells.find(to_upper(i->name.GetString()));
        if (spellIter == std::end(allSpells)) {
            std::cerr << "spells: unknown type '" << i->name.GetString() <<
                "'\n";
            continue;
        }

        auto type = spellIter->second;
        cache.emplace(static_cast<int>(type),
                      make_unique<Spell>(type, i->value));
    }

    return true;
}

const Spell * getSpell(SpellType type)
{
    auto iter = cache.find(static_cast<int>(type));
    if (iter == std::end(cache)) return nullptr;
    return iter->second.get();
}

const Spell * getSpell(const std::string &type)
{
    auto iter = allSpells.find(to_upper(type));
    if (iter == std::end(allSpells)) return nullptr;
    return getSpell(iter->second);
}
