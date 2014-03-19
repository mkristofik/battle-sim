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
#ifndef SPELLS_H
#define SPELLS_H

#include "Effects.h"
#include <string>

#define SPELL_TYPES \
    X(LIGHTNING)

#define SPELL_TARGETS \
    X(ENEMY) \
    X(FRIEND) \
    X(ANY) \
    X(ALL)

#define X(str) str,
enum class SpellType {SPELL_TYPES};
enum class SpellTarget {SPELL_TARGETS};
#undef X

struct Spell
{
    std::string name;
    int damage;
    SpellType type;
    SpellTarget target;
    EffectType effect;

    Spell(SpellType t);
};

// Call this after SDL initialized but before the game data initialized.
// TODO: enforce this in code
void initSpellCache();

const Spell * getSpell(SpellType type);
const Spell * getSpell(const std::string &type);

#endif
