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
#include <algorithm>
#include <cassert>
#include <sstream>

namespace
{
    int maxWidth(const SdlSurface &a, const SdlSurface &b)
    {
        if (!a && !b) return 0;
        if (!a) return b->w;
        if (!b) return a->w;
        return std::max(a->w, b->w);
    }

    int maxWidth(const SdlSurface &a, int maxSoFar)
    {
        if (!a) return maxSoFar;
        return std::max(a->w, maxSoFar);
    }

    int maxWidth(const SdlSurface &a, const SdlSurface &b, const SdlSurface &c)
    {
        return maxWidth(a, maxWidth(b, c));
    }
}

UnitView::UnitView()
{
}

SdlSurface UnitView::render(const Unit &unit) const
{
    assert(unit.isAlive());

    // Render all the pieces to determine overall window size.
    const auto &img = unit.type->baseImg[unit.team];
    auto nameSurf = renderName(unit);
    auto damageSurf = renderDamage(unit);
    auto hpSurf = renderHP(unit);
    auto traitSurf = renderTraits(unit);
    auto effectSurf = renderEffects(unit);

    int topWidth = img->w + maxWidth(nameSurf, damageSurf, hpSurf);
    int bottomWidth = maxWidth(traitSurf, effectSurf);
    int width = std::max(topWidth, bottomWidth) + 5;  // allow buffer space

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

SdlSurface UnitView::renderName(const Unit &unit) const
{
    const auto &nameFont = sdlGetFont(FontType::LARGE);
    return sdlPreRender(nameFont, unit.getName(), WHITE);
}

SdlSurface UnitView::renderDamage(const Unit &unit) const
{
    std::ostringstream dmg{"Damage ", std::ios::app};
    if (unit.hasTrait(Trait::RANGED)) {
        dmg << unit.type->minDmgRanged << '-' << unit.type->maxDmgRanged;
    }
    else {
        dmg << unit.type->minDmg << '-' << unit.type->maxDmg;
    }

    return sdlPreRender(sdlGetFont(FontType::MEDIUM), dmg.str(), WHITE);
}

SdlSurface UnitView::renderHP(const Unit &unit) const
{
    const auto &font = sdlGetFont(FontType::MEDIUM);
    auto lblHP = sdlPreRender(font, "HP ", WHITE);

    auto hpColor = WHITE;
    if (static_cast<double>(unit.hpLeft) / unit.type->hp < 0.25) {
        hpColor = RED;
    }
    else if (static_cast<double>(unit.hpLeft) / unit.type->hp < 0.5) {
        hpColor = YELLOW;
    }
    auto lblHpLeft = sdlPreRender(font, unit.hpLeft, hpColor);

    std::ostringstream ostr{" / ", std::ios::app};
    ostr << unit.type->hp;
    auto lblHpTot = sdlPreRender(font, ostr.str(), WHITE);

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

SdlSurface UnitView::renderTraits(const Unit &unit) const
{
    auto str = strFromTraits(unit.type->traits);
    if (str.empty()) return {};

    // TODO: need something for spellcasters
    return sdlPreRender(sdlGetFont(FontType::MEDIUM), str, WHITE);
}

SdlSurface UnitView::renderEffects(const Unit &unit) const
{
    if (unit.effect.type == EffectType::NONE) return {};
    return sdlPreRender(sdlGetFont(FontType::MEDIUM), unit.effect.getText(),
                        YELLOW);
}
