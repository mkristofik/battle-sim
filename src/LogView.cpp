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

namespace
{
    auto white = SDL_Color{255, 255, 255, 0};
}

LogView::LogView(SDL_Rect dispArea, const SdlFont &f)
    : displayArea_(std::move(dispArea)),
    font_(f),
    msgs_{},
    curMsg_{0}
{
}

void LogView::add(std::string msg)
{
    Message m(std::move(msg));

    // Render the message off-screen to find out how big it is.
    SDL_Rect temp = displayArea_;
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

    // The descender on lowercase g tends to get cut off between messages.
    // Increase the spacing to allow for it.
    auto lineHeight = TTF_FontLineSkip(font_.get()) + 1;

    // Compute how many messages will fit in the log area.
    int maxLines = displayArea_.h / lineHeight;
    int linesToDraw = 0;
    auto startIdx = msgs_.size();
    for (auto i = msgs_.rbegin(); i != msgs_.rend(); ++i) {
        if (linesToDraw + i->lines > maxLines) {
            break;
        }
        linesToDraw += i->lines;
        --startIdx;
    }

    // Draw them.
    sdlClear(displayArea_);
    SDL_Rect drawTarget = displayArea_;
    for (auto i = startIdx; i < msgs_.size(); ++i) {
        int lines = sdlDrawText(font_, msgs_[i].msg, drawTarget, white);
        drawTarget.y += lines * lineHeight;
        drawTarget.h -= lines * lineHeight;
    }
}
