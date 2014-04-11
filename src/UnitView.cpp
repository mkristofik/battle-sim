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
#include <sstream>

UnitView::UnitView(SDL_Rect dispArea, int team, const GameState &gs)
    : displayArea_(std::move(dispArea)),
    team_(team),
    gs_(gs),
    font_(sdlGetFont(FontType::MEDIUM))
{
}

void UnitView::draw() const
{
    sdlClear(displayArea_);

    const auto &unit = gs_.getActiveUnit();
    if (!unit.isAlive() || unit.team != team_) return;

    const auto &img = unit.type->baseImg[team_];
    sdlBlit(img, displayArea_.x, displayArea_.y);

    auto px = displayArea_.x + img->w + 5;
    auto py = displayArea_.y + 5;
    sdlBlit(renderName(unit), px, py);

    auto lineHeight = sdlLineHeight(font_);
    py += lineHeight;
    sdlBlit(renderDamage(unit), px, py);

    py += lineHeight;
    sdlBlit(renderHP(unit), px, py);

    auto traitSurf = renderTraits(unit);
    px = displayArea_.x + 5;
    py = displayArea_.y + img->h;
    if (traitSurf != nullptr) {
        sdlBlit(traitSurf, px, py);
        py += lineHeight;
    }

    auto effectSurf = renderEffects(unit);
    if (effectSurf != nullptr) {
        sdlBlit(effectSurf, px, py);
    }
}

SdlSurface UnitView::renderName(const Unit &unit) const
{
    return sdlPreRender(font_, unit.getName(), WHITE);
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

    return sdlPreRender(font_, dmg.str(), WHITE);
}

SdlSurface UnitView::renderHP(const Unit &unit) const
{
    auto lblHP = sdlPreRender(font_, "HP ", WHITE);

    auto hpColor = WHITE;
    if (static_cast<double>(unit.hpLeft) / unit.type->hp < 0.25) {
        hpColor = RED;
    }
    else if (static_cast<double>(unit.hpLeft) / unit.type->hp < 0.5) {
        hpColor = YELLOW;
    }
    auto lblHpLeft = sdlPreRender(font_, unit.hpLeft, hpColor);

    std::ostringstream ostr{" / ", std::ios::app};
    ostr << unit.type->hp;
    auto lblHpTot = sdlPreRender(font_, ostr.str(), WHITE);

    SDL_Rect dest = {0};
    auto width = lblHP->w + lblHpLeft->w + lblHpTot->w;
    auto hpSurf = sdlCreate(width, sdlLineHeight(font_));
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
    return sdlPreRender(font_, str, WHITE);
}

SdlSurface UnitView::renderEffects(const Unit &unit) const
{
    if (unit.effect.type == EffectType::NONE) return {};
    return sdlPreRender(font_, unit.effect.getText(), YELLOW);
}
