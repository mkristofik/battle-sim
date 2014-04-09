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

#include "GameState.h"
#include "UnitType.h"
#include <sstream>
/*
 * -------
 * |     |  23 Archers
 * |     |  Dmg 2-3
 * |     |  HP 8/10   (make first # yellow if < 50%, red if < 25%)
 * -------
 */

UnitView::UnitView(SDL_Rect dispArea, int team, const GameState &gs,
                   const SdlFont &f)
    : displayArea_(std::move(dispArea)),
    team_(team),
    gs_(gs),
    font_(f)
{
}

void UnitView::draw() const
{
    sdlClear(displayArea_);

    const auto &unit = gs_.getActiveUnit();
    if (!unit.isAlive() || unit.team != team_) return;

    const auto &img = unit.type->baseImg[team_];
    sdlBlit(img, displayArea_.x, displayArea_.y);

    // Draw unit name and size.
    auto lineHeight = sdlLineHeight(font_);
    SDL_Rect txtArea;
    txtArea.x = displayArea_.x + img->w + 5;
    txtArea.y = displayArea_.y + 5;
    txtArea.w = displayArea_.w - img->w - 10;
    txtArea.h = sdlLineHeight(font_);
    sdlDrawText(font_, unit.getName(), txtArea, WHITE);

    // Damage
    txtArea.y += lineHeight + 2;
    std::ostringstream dmg{"Damage ", std::ios::app};
    if (unit.hasTrait(Trait::RANGED)) {
        dmg << unit.type->minDmgRanged << '-' << unit.type->maxDmgRanged;
    }
    else {
        dmg << unit.type->minDmg << '-' << unit.type->maxDmg;
    }
    sdlDrawText(font_, dmg.str(), txtArea, WHITE);

    // HP
    txtArea.y += lineHeight + 1;
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
    sdlBlit(lblHP, txtArea.x, txtArea.y);
    sdlBlit(lblHpLeft, txtArea.x + lblHP->w, txtArea.y);
    sdlBlit(lblHpTot, txtArea.x + lblHP->w + lblHpLeft->w, txtArea.y);

    // Traits
    txtArea.x = displayArea_.x + 5;
    txtArea.y = displayArea_.y + img->h;
    txtArea.w = displayArea_.w - 10;
    txtArea.h = sdlLineHeight(font_) * 2;
    auto lines = sdlDrawText(font_, strFromTraits(unit.type->traits), txtArea,
                             WHITE);

    // Effects
    if (unit.effect.type != EffectType::NONE) {
        txtArea.y += lines * lineHeight;
        sdlDrawText(font_, unit.effect.getText(), txtArea, YELLOW);
    }
}
