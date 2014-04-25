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
#ifndef LOGVIEW_H
#define LOGVIEW_H

#include "sdl_helper.h"
#include <string>
#include <vector>

// Draw the battle log that records every attack and spell cast.

class LogView
{
public:
    LogView(SDL_Rect dispArea);
    void add(std::string msg);
    void addBlankLine();

    void draw();
    void handleMouseDown(const SDL_MouseButtonEvent &event);
    void handleMouseUp(const SDL_MouseButtonEvent &event);

private:
    // Make sure the most recent message is visible.
    void scrollToEnd();

    void updateMsgLimits();
    void updateButtonState();
    void drawButtons() const;

    struct Message
    {
        std::string msg;
        int lines;

        Message(std::string &&m) : msg(std::move(m)), lines{0} {}
    };

    enum class ButtonState { READY, PRESSED, DISABLED };

    SDL_Rect displayArea_;
    SdlSurface btnUp_;
    SdlSurface btnUpPressed_;
    SdlSurface btnUpDisabled_;
    SDL_Rect btnUpArea_;
    ButtonState upState_;
    SdlSurface btnDown_;
    SdlSurface btnDownPressed_;
    SdlSurface btnDownDisabled_;
    SDL_Rect btnDownArea_;
    ButtonState downState_;
    SDL_Rect textArea_;
    const SdlFont &font_;
    int lineHeight_;
    int maxLines_;
    std::vector<Message> msgs_;
    int beginMsg_;  // first visible message
    int endMsg_;    // one past the last visible message (like an iterator)
};

#endif
