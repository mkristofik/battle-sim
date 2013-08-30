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
#ifndef LOGVIEW_H
#define LOGVIEW_H

#include "sdl_helper.h"
#include <string>
#include <vector>

// Draw the battle log that records every attack and spell cast.

class LogView
{
public:
    LogView(SDL_Rect dispArea, const SdlFont &f);
    void add(std::string msg);

    void draw() const;

private:
    struct Message
    {
        std::string msg;
        int lines;

        Message(std::string &&m) : msg(std::move(m)), lines{0} {}
    };

    SDL_Rect displayArea_;
    SdlSurface btnUp_;
    SdlSurface btnDown_;
    SDL_Rect textArea_;
    const SdlFont &font_;
    int lineHeight_;
    std::vector<Message> msgs_;
    int curMsg_;
};

#endif
