//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright (C) 2015-2019 Daniel Allendorf, Ryan Payton                    //
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

namespace jrc
{
    namespace KeyConfig
    {
        enum Key : uint8_t
        {
            NUM1 = 2,
            NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9, NUM0, MINUS, EQUAL,
            Q = 16,
            W, E, R, T, Y, U, I, O, P, LEFT_BRACKET, RIGHT_BRACKET,
            LEFT_CONTROL = 29,
            A, S, D, F, G, H, J, K, L, SEMICOLON, APOSTROPHE, GRAVE_ACCENT, LEFT_SHIFT, BACKSLASH, Z, X, C, V, B, N, M, COMMA, PERIOD,
            LEFT_ALT = 56,
            SPACE,
            F1 = 59,
            F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, HOME,
            PAGE_UP = 73,
            END = 79,
            PAGE_DOWN = 81,
            INSERT, DELETE, ESCAPE, RIGHT_CONTROL, RIGHT_SHIFT, RIGHT_ALT, SCROLL_LOCK,
            LENGTH
        };

        inline Key actionbyid(int32_t id)
        {
            return static_cast<Key>(id);
        }
    }
}

