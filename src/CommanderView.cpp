/*
    Copyright (C) 2013 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "CommanderView.h"

#include "Commander.h"
#include "boost/lexical_cast.hpp"
#include <string>

CommanderView::CommanderView(const Commander &c, int team, const SdlFont &f,
                             SDL_Rect dispArea)
    : cmdr_(c),
    font_(f),
    displayArea_(std::move(dispArea)),
    txtAlign_{team == 0 ? Justify::LEFT : Justify::RIGHT}
{
}

void CommanderView::draw() const
{
    sdlClear(displayArea_);

    // Draw the portrait.
    auto img = cmdr_.portrait;
    if (txtAlign_ == Justify::RIGHT) {
        img = sdlFlipH(img);
    }
    sdlBlit(img, displayArea_.x, displayArea_.y);

    // Draw the name below it.
    auto lineHeight = TTF_FontLineSkip(font_.get()) + 1;  // TODO move this
    SDL_Rect txtArea;
    txtArea.x = displayArea_.x + 5;  // get away from the screen edges a bit
    txtArea.y = displayArea_.y + img->h;
    txtArea.w = displayArea_.w - 10;
    txtArea.h = lineHeight;
    auto title = cmdr_.name + " (" + cmdr_.alignment + ")";
    sdlDrawText(font_, title, txtArea, WHITE, txtAlign_);

    // Draw the stats below the name.
    txtArea.y += lineHeight;
    std::string stats = "Attack: " +
        boost::lexical_cast<std::string>(cmdr_.attack) +
        "  Defense: " +
        boost::lexical_cast<std::string>(cmdr_.defense);
    sdlDrawText(font_, stats, txtArea, WHITE, txtAlign_);
}
