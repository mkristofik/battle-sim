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
#include <cassert>

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
    btnUpPressed_{sdlLoadImage("button-up-pressed.png")},
    btnUpDisabled_{sdlLoadImage("button-up-disabled.png")},
    upState_{ButtonState::DISABLED},
    btnDown_{sdlLoadImage("button-down.png")},
    btnDownPressed_{sdlLoadImage("button-down-pressed.png")},
    btnDownDisabled_{sdlLoadImage("button-down-disabled.png")},
    downState_{ButtonState::DISABLED},
    textArea_(displayArea_),
    font_(f),
    lineHeight_{TTF_FontLineSkip(font_.get())},
    maxLines_{0},
    msgs_{},
    beginMsg_{0},
    endMsg_{0}
{
    // Allow space for the scroll buttons.
    textArea_.w -= btnUp_->w;

    // The descender on lowercase g tends to get cut off between messages.
    // Increase the spacing to allow for it.
    ++lineHeight_;

    maxLines_ = textArea_.h / lineHeight_;
}

void LogView::add(std::string msg)
{
    Message m(std::move(msg));

    // Render the message off-screen to find out how big it is.
    SDL_Rect temp = textArea_;
    temp.x = -temp.w;
    m.lines = sdlDrawText(font_, m.msg, temp, white);

    msgs_.emplace_back(std::move(m));
    scrollToEnd();
}

void LogView::draw() const
{
    // TODO: add a dirty flag.
    if (msgs_.empty()) {
        return;
    }

    sdlClear(displayArea_);
    drawButtons();

    // Display the messages.
    SDL_Rect drawTarget = textArea_;
    for (auto i = beginMsg_; i < endMsg_; ++i) {
        sdlDrawText(font_, msgs_[i].msg, drawTarget, white);
        drawTarget.y += msgs_[i].lines * lineHeight_;
        drawTarget.h -= msgs_[i].lines * lineHeight_;
    }
}

void LogView::scrollToEnd()
{
    assert(!msgs_.empty());

    endMsg_ = msgs_.size();
    beginMsg_ = endMsg_ - 1;
    int numLines = msgs_[beginMsg_].lines;
    while (beginMsg_ > 0 && numLines + msgs_[beginMsg_ - 1].lines <= maxLines_)
    {
        --beginMsg_;
        numLines += msgs_[beginMsg_].lines;
    }
    updateButtonState();
}

void LogView::updateButtonState()
{
    upState_ = (beginMsg_ > 0) ? ButtonState::READY : ButtonState::DISABLED;
    int size = msgs_.size();
    downState_ = (endMsg_ < size) ? ButtonState::READY : ButtonState::DISABLED;
}

void LogView::drawButtons() const
{
    // Draw the up and down arrow buttons on the right edge.
    auto px = displayArea_.x + displayArea_.w - btnUp_->w;
    auto downY = displayArea_.y + displayArea_.h - btnDown_->h;

    switch (upState_) {
        case ButtonState::READY:
            sdlBlit(btnUp_, px, displayArea_.y);
            break;
        case ButtonState::PRESSED:
            sdlBlit(btnUpPressed_, px, displayArea_.y);
            break;
        case ButtonState::DISABLED:
            sdlBlit(btnUpDisabled_, px, displayArea_.y);
            break;
    }
    switch (downState_) {
        case ButtonState::READY:
            sdlBlit(btnDown_, px, downY);
            break;
        case ButtonState::PRESSED:
            sdlBlit(btnDownPressed_, px, downY);
            break;
        case ButtonState::DISABLED:
            sdlBlit(btnDownDisabled_, px, downY);
            break;
    }
}
