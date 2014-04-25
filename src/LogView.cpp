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
#include "LogView.h"
#include <cassert>

// TODO: future ideas
// Mouse wheel buttons are SDL_BUTTON_WHEELUP and SDL_BUTTON_WHEELDOWN.  It
// doesn't take much to generate a lot of events with the wheel.

LogView::LogView(SDL_Rect dispArea)
    : displayArea_(std::move(dispArea)),
    btnUp_{sdlLoadImage("button-up.png")},
    btnUpPressed_{sdlLoadImage("button-up-pressed.png")},
    btnUpDisabled_{sdlLoadImage("button-up-disabled.png")},
    btnUpArea_{},
    upState_{ButtonState::DISABLED},
    btnDown_{sdlLoadImage("button-down.png")},
    btnDownPressed_{sdlLoadImage("button-down-pressed.png")},
    btnDownDisabled_{sdlLoadImage("button-down-disabled.png")},
    btnDownArea_{},
    downState_{ButtonState::DISABLED},
    textArea_(displayArea_),
    font_(sdlGetFont(FontType::MEDIUM)),
    lineHeight_{sdlLineHeight(font_)},
    maxLines_{textArea_.h / lineHeight_},
    msgs_{},
    beginMsg_{0},
    endMsg_{0}
{
    // Allow space for the scroll buttons.
    textArea_.w -= btnUp_->w;

    // Draw the arrow buttons on the right edge.
    btnUpArea_.x = displayArea_.x + displayArea_.w - btnUp_->w;
    btnUpArea_.y = displayArea_.y;
    btnUpArea_.w = btnUp_->w;
    btnUpArea_.h = btnUp_->h;
    btnDownArea_.x = btnUpArea_.x;
    btnDownArea_.y = displayArea_.y + displayArea_.h - btnDown_->h;
    btnDownArea_.w = btnDown_->w;
    btnDownArea_.h = btnDown_->h;
}

void LogView::add(std::string msg)
{
    Message m{std::move(msg)};
    m.lines = sdlWordWrap(font_, m.msg, textArea_.w).size();
    msgs_.emplace_back(std::move(m));
    scrollToEnd();
}

void LogView::addBlankLine()
{
    add(" ");
}

void LogView::draw()
{
    sdlClear(displayArea_);
    drawButtons();

    // Display the messages.
    SDL_Rect drawTarget = textArea_;
    for (auto i = beginMsg_; i < endMsg_; ++i) {
        sdlDrawText(font_, msgs_[i].msg, drawTarget, WHITE);
        drawTarget.y += msgs_[i].lines * lineHeight_;
        drawTarget.h -= msgs_[i].lines * lineHeight_;
    }
}

void LogView::handleMouseDown(const SDL_MouseButtonEvent &event)
{
    if (insideRect(event.x, event.y, btnUpArea_) &&
        upState_ == ButtonState::READY)
    {
        upState_ = ButtonState::PRESSED;
    }
    else if (insideRect(event.x, event.y, btnDownArea_) &&
             downState_ == ButtonState::READY)
    {
        downState_ = ButtonState::PRESSED;
    }
}

void LogView::handleMouseUp(const SDL_MouseButtonEvent &event)
{
    if (upState_ == ButtonState::PRESSED) {
        --beginMsg_;
        updateMsgLimits();
        updateButtonState();
    }
    else if (downState_ == ButtonState::PRESSED) {
        ++beginMsg_;
        updateMsgLimits();
        updateButtonState();
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

void LogView::updateMsgLimits()
{
    if (msgs_.empty()) {
        return;
    }

    int totLines = msgs_[beginMsg_].lines;
    endMsg_ = beginMsg_ + 1;
    while (endMsg_ < static_cast<int>(msgs_.size())) {
        int nextLines = msgs_[endMsg_].lines;
        if (totLines + nextLines > maxLines_) {
           break;
        }
        totLines += nextLines;
        ++endMsg_;
    }
}

void LogView::updateButtonState()
{
    upState_ = (beginMsg_ > 0) ? ButtonState::READY : ButtonState::DISABLED;
    int size = msgs_.size();
    downState_ = (endMsg_ < size) ? ButtonState::READY : ButtonState::DISABLED;
}

void LogView::drawButtons() const
{
    switch (upState_) {
        case ButtonState::READY:
            sdlBlit(btnUp_, btnUpArea_.x, btnUpArea_.y);
            break;
        case ButtonState::PRESSED:
            sdlBlit(btnUpPressed_, btnUpArea_.x, btnUpArea_.y);
            break;
        case ButtonState::DISABLED:
            sdlBlit(btnUpDisabled_, btnUpArea_.x, btnUpArea_.y);
            break;
    }
    switch (downState_) {
        case ButtonState::READY:
            sdlBlit(btnDown_, btnDownArea_.x, btnDownArea_.y);
            break;
        case ButtonState::PRESSED:
            sdlBlit(btnDownPressed_, btnDownArea_.x, btnDownArea_.y);
            break;
        case ButtonState::DISABLED:
            sdlBlit(btnDownDisabled_, btnDownArea_.x, btnDownArea_.y);
            break;
    }
}
