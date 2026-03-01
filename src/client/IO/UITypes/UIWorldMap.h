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
#include "../UIDragElement.h"

#include "../Components/AreaButton.h"
#include "../Components/Textfield.h"

#include "../../Graphics/Texture.h"

#include <map>
#include <string>
#include <vector>

namespace jrc
{
    class UIWorldMap : public UIDragElement<PosMAP>
    {
    public:
        static constexpr Type TYPE = UIElement::WORLDMAP;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        UIWorldMap();

        void draw(float inter) const override;
        void update() override;
        void toggle_active() override;

        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
        void send_key(int32_t keycode, bool pressed, bool escape);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        static constexpr uint8_t MAPSPOT_TYPE_MAX = 4u;

        struct MapSpot
        {
            std::string description;
            Texture path;
            std::string title;
            uint8_t type;
            Texture marker;
            bool bolded;
            std::vector<int32_t> map_ids;
        };

        void set_search(bool enable);
        void update_world(const std::string& parent_map);
        Point<int16_t> get_search_pos() const;

        enum Buttons
        {
            BT_CLOSE,
            BT_SEARCH,
            BT_AUTOFLY,
            BT_NAVIREG,
            BT_ALLSEARCH,
            BT_SEARCH_CLOSE,
            BT_LINK0,
            BT_LINK1,
            BT_LINK2,
            BT_LINK3,
            BT_LINK4,
            BT_LINK5,
            BT_LINK6,
            BT_LINK7,
            BT_LINK8,
            BT_LINK9
        };

        bool search;
        bool show_path_img;
        int32_t mapid;
        std::string parent_map;
        std::string user_map;

        Texture search_background;
        Texture search_notice;
        Texture base_img;
        Texture path_img;

        Animation cur_pos;
        Animation npc_pos[MAPSPOT_TYPE_MAX];

        Textfield search_text;

        std::map<uint16_t, Texture> link_images;
        std::map<uint16_t, std::string> link_maps;
        std::vector<std::pair<Point<int16_t>, MapSpot>> map_spots;

        Point<int16_t> bg_dimensions;
        Point<int16_t> bg_search_dimensions;
        Point<int16_t> background_dimensions;
        Point<int16_t> base_position;
        Point<int16_t> search_text_dim;
    };
}
