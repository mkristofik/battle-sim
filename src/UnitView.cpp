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
#include "UnitView.h"

#include "Effects.h"
#include "GameState.h"
#include "UnitType.h"
#include "algo.h"
#include <algorithm>
#include <cassert>
#include <sstream>

namespace
{
    // Return true if lhs is narrower than rhs.  A null surface is treated like
    // it has infinite width.
    bool compareWidth(const SdlSurface &lhs, const SdlSurface &rhs)
    {
        if (!lhs) return false;
        if (!rhs) return true;
        return lhs->w < rhs->w;
    }

    SdlSurface renderName(const Unit &unit)
    {
        const auto &nameFont = sdlGetFont(FontType::LARGE);
        return sdlRenderText(nameFont, unit.getName(), WHITE);
    }

    SdlSurface renderDamage(const Unit &unit)
    {
        std::ostringstream dmg{"Damage ", std::ios::app};
        if (unit.hasTrait(Trait::RANGED)) {
            dmg << unit.type->minDmgRanged << '-' << unit.type->maxDmgRanged;
        }
        else {
            dmg << unit.type->minDmg << '-' << unit.type->maxDmg;
        }

        return sdlRenderText(sdlGetFont(FontType::MEDIUM), dmg.str(), WHITE);
    }

    SdlSurface renderHP(const Unit &unit)
    {
        const auto &font = sdlGetFont(FontType::MEDIUM);
        auto lblHP = sdlRenderText(font, "HP ", WHITE);

        auto hpColor = WHITE;
        if (static_cast<double>(unit.hpLeft) / unit.type->hp < 0.25) {
            hpColor = RED;
        }
        else if (static_cast<double>(unit.hpLeft) / unit.type->hp < 0.5) {
            hpColor = YELLOW;
        }
        auto lblHpLeft = sdlRenderText(font, unit.hpLeft, hpColor);

        std::ostringstream ostr{" / ", std::ios::app};
        ostr << unit.type->hp;
        auto lblHpTot = sdlRenderText(font, ostr.str(), WHITE);

        SDL_Rect dest = {0};
        auto width = lblHP->w + lblHpLeft->w + lblHpTot->w;
        auto hpSurf = sdlCreate(width, sdlLineHeight(font));
        SDL_BlitSurface(lblHP.get(), nullptr, hpSurf.get(), nullptr);
        dest.x = lblHP->w;
        SDL_BlitSurface(lblHpLeft.get(), nullptr, hpSurf.get(), &dest);
        dest.x += lblHpLeft->w;
        SDL_BlitSurface(lblHpTot.get(), nullptr, hpSurf.get(), &dest);

        return hpSurf;
    }

    SdlSurface renderSpellTraits(const Unit &unit)
    {
        assert(contains(unit.type->traits, Trait::SPELLCASTER));

        // TODO: building up text like this is awkward.
        auto str = strFromTraits(unit.type->traits);
        auto line1 = sdlRenderText(sdlGetFont(FontType::MEDIUM), str, WHITE);
        std::ostringstream spellStr;
        spellStr << "1. " << unit.type->spell->name << " (" <<
            unit.type->spell->cost << ')';
        auto line2 = sdlRenderText(sdlGetFont(FontType::MEDIUM), spellStr.str(),
                                   WHITE);
        auto width = std::max(line1, line2, compareWidth)->w;
        auto height = line1->h + line2->h;

        auto surf = sdlCreate(width, height);
        SDL_BlitSurface(line1.get(), nullptr, surf.get(), nullptr);
        SDL_Rect dest2 = {0};
        dest2.y = line1->h;
        SDL_BlitSurface(line2.get(), nullptr, surf.get(), &dest2);
        return surf;
    }

    SdlSurface renderTraits(const Unit &unit)
    {
        const auto &traits = unit.type->traits;
        if (traits.empty()) return {};
        if (contains(traits, Trait::SPELLCASTER)) {
            return renderSpellTraits(unit);
        }

        auto str = strFromTraits(traits);
        return sdlRenderText(sdlGetFont(FontType::MEDIUM), str, WHITE);
    }

    SdlSurface renderEffects(const Unit &unit)
    {
        if (unit.effect.type == EffectType::NONE) return {};
        return sdlRenderText(sdlGetFont(FontType::MEDIUM),
                             unit.effect.getText(), YELLOW);
    }
}

SdlSurface renderUnitView(const Unit &unit)
{
    assert(unit.isAlive());

    // Render all the pieces to determine overall window size.
    const auto &img = unit.type->baseImg[unit.team];
    auto nameSurf = renderName(unit);
    auto damageSurf = renderDamage(unit);
    auto hpSurf = renderHP(unit);
    auto traitSurf = renderTraits(unit);
    auto effectSurf = renderEffects(unit);

    // How wide do we need to be?
    auto widest = std::max({nameSurf, damageSurf, hpSurf}, compareWidth);
    int topWidth = img->w + widest->w;
    widest = std::max(traitSurf, effectSurf, compareWidth);
    int bottomWidth = (widest ? widest->w : 0);
    int width = std::max(topWidth, bottomWidth) + 5;  // allow buffer space

    // How tall?
    int height = img->h;
    if (traitSurf) height += traitSurf->h;
    if (effectSurf) height += effectSurf->h;

    auto surf = sdlCreate(width, height);
    SDL_Rect dest = {0};

    SDL_BlitSurface(img.get(), nullptr, surf.get(), nullptr);
    dest.x = img->w + 5;
    dest.y = 5;
    SDL_BlitSurface(nameSurf.get(), nullptr, surf.get(), &dest);
    dest.y += nameSurf->h + 5;
    SDL_BlitSurface(damageSurf.get(), nullptr, surf.get(), &dest);
    dest.y += damageSurf->h;
    SDL_BlitSurface(hpSurf.get(), nullptr, surf.get(), &dest);
    dest.x = 5;
    dest.y = img->h;
    if (traitSurf) {
        SDL_BlitSurface(traitSurf.get(), nullptr, surf.get(), &dest);
        dest.y += traitSurf->h;
    }
    if (effectSurf) {
        SDL_BlitSurface(effectSurf.get(), nullptr, surf.get(), &dest);
    }

    return surf;
}
