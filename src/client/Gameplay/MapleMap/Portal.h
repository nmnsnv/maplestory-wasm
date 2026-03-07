//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright © 2015-2016 Daniel Allendorf                                   //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../../Graphics/Animation.h"
#include "../../Template/Rectangle.h"

#include <cstdint>
#include <map>
#include <utility>


namespace jrc
{
    class Portal
    {
    public:
        enum Type
        {
            SPAWN,
            INVISIBLE,
            REGULAR,
            TOUCH,
            TYPE4,
            TYPE5,
            WARP,
            SCRIPTED,
            SCRIPTED_INVISIBLE,
            SCRIPTED_TOUCH,
            HIDDEN,
            SCRIPTED_HIDDEN,
            SPRING1,
            SPRING2,
            TYPE14
        };

        static Type typebyid(int32_t id)
        {
            return static_cast<Type>(id);
        }

        struct WarpInfo
        {
            static constexpr int32_t INVALID_MAP_ID = 999999999;

            int32_t mapid;
            std::string toname;
            std::string name;
            bool intramap;
            bool valid;

            bool has_target_map() const
            {
                return mapid >= 0 && mapid < INVALID_MAP_ID;
            }

            WarpInfo(int32_t m, Type portal_type, bool i, std::string tn, std::string n)
                : mapid(m), toname(std::move(tn)), name(std::move(n)), intramap(i), valid(false)
            {
                bool scripted = portal_type == SCRIPTED ||
                    portal_type == SCRIPTED_INVISIBLE ||
                    portal_type == SCRIPTED_TOUCH ||
                    portal_type == SCRIPTED_HIDDEN;

                valid = has_target_map() || scripted;
            }

            WarpInfo()
                : WarpInfo(INVALID_MAP_ID, SPAWN, false, {}, {}) {}
        };

        Portal(const Animation* animation,
               Type type,
               std::string name,
               bool intramap,
               Point<int16_t> position,
               int32_t tomap,
               std::string toname);
        Portal();

        void update(Point<int16_t> playerpos);
        void draw(Point<int16_t> viewpos, float alpha) const;

        std::string get_name() const;
        Type get_type() const;
        Point<int16_t> get_position() const;
        Rectangle<int16_t> bounds() const;

        WarpInfo getwarpinfo() const;

    private:
        const Animation* animation;
        Type type;
        std::string name;
        Point<int16_t> position;
        WarpInfo warpinfo;
        bool touched;
    };
}
