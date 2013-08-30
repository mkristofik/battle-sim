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
#include "LogView.h"

// TODO: future ideas
// Add images for when scroll buttons are clicked or disabled.
// Set up click regions slightly bigger than the buttons themselves.
// When a button is clicked, scroll 1 message in that direction.
// Need mouse click event handler.  Dispatch from battle.cpp.
// Look into whether we can support the mouse wheel.
namespace
{
    auto white = SDL_Color{255, 255, 255, 0};
}

LogView::LogView(SDL_Rect dispArea, const SdlFont &f)
    : displayArea_(std::move(dispArea)),
    btnUp_{sdlLoadImage("button-up.png")},
    btnDown_{sdlLoadImage("button-down.png")},
    textArea_(displayArea_),
    font_(f),
    lineHeight_{TTF_FontLineSkip(font_.get())},
    msgs_{},
    curMsg_{0}
{
    // Allow space for the scroll buttons.
    textArea_.w -= btnUp_->w;

    // The descender on lowercase g tends to get cut off between messages.
    // Increase the spacing to allow for it.
    ++lineHeight_;
}

void LogView::add(std::string msg)
{
    Message m(std::move(msg));

    // Render the message off-screen to find out how big it is.
    SDL_Rect temp = textArea_;
    temp.x = -temp.w;
    m.lines = sdlDrawText(font_, m.msg, temp, white);

    msgs_.emplace_back(std::move(m));
    curMsg_ = static_cast<int>(msgs_.size()) - 1;
}

void LogView::draw() const
{
    if (msgs_.empty()) {
        return;
    }

    // Compute how many messages will fit in the log area.
    int maxLines = displayArea_.h / lineHeight_;
    int linesToDraw = 0;
    auto startIdx = msgs_.size();
    for (auto i = msgs_.rbegin(); i != msgs_.rend(); ++i) {
        if (linesToDraw + i->lines > maxLines) {
            break;
        }
        linesToDraw += i->lines;
        --startIdx;
    }

    sdlClear(displayArea_);

    // Draw the up and down arrow buttons on the right edge.
    auto px = displayArea_.x + displayArea_.w - btnUp_->w;
    auto downY = displayArea_.y + displayArea_.h - btnDown_->h;
    sdlBlit(btnUp_, px, displayArea_.y);
    sdlBlit(btnDown_, px, downY);

    // Display the messages.
    SDL_Rect drawTarget = textArea_;
    for (auto i = startIdx; i < msgs_.size(); ++i) {
        int lines = sdlDrawText(font_, msgs_[i].msg, drawTarget, white);
        drawTarget.y += lines * lineHeight_;
        drawTarget.h -= lines * lineHeight_;
    }
}
