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

#include "../Components/Slider.h"

#include "../../Character/CharStats.h"
#include "../../Gameplay/Stage.h"

#include <utility>
#include <vector>

namespace jrc
{
    class UIMiniMap : public UIDragElement<PosMINIMAP>
    {
    public:
        static constexpr Type TYPE = UIElement::MINIMAP;
        static constexpr bool FOCUSED = false;
        static constexpr bool TOGGLED = true;

        explicit UIMiniMap(const CharStats& stats);

        void draw(float alpha) const override;
        void update() override;

        bool remove_cursor(bool clicked, Point<int16_t> cursorpos) override;
        CursorResult send_cursor(bool clicked, Point<int16_t> cursorpos) override;
        void send_scroll(double yoffset) override;
        void send_key(int32_t keycode, bool pressed, bool escape);

    protected:
        Button::State button_pressed(uint16_t buttonid) override;

    private:
        static constexpr int16_t CENTER_START_X = 64;
        static constexpr int16_t BTN_MIN_Y = 4;
        static constexpr int16_t ML_MR_Y = 17;
        static constexpr int16_t MAX_ADJ = 40;
        static constexpr int16_t M_START = 36;
        static constexpr int16_t LISTNPC_ITEM_HEIGHT = 17;
        static constexpr int16_t LISTNPC_ITEM_WIDTH = 140;
        static constexpr int16_t LISTNPC_TEXT_WIDTH = 114;

        enum Buttons
        {
            BT_MIN,
            BT_MAX,
            BT_SMALL,
            BT_BIG,
            BT_MAP,
            BT_NPC
        };

        enum WindowType
        {
            MIN,
            NORMAL,
            MAX
        };

        void update_buttons();
        void toggle_buttons();
        void update_text();
        void update_canvas();
        void draw_movable_markers(Point<int16_t> init_pos, float alpha) const;
        void update_static_markers();
        void set_npclist_active(bool active);
        void update_dimensions();
        void update_npclist();
        void draw_npclist(Point<int16_t> minimap_dims, float alpha) const;
        void select_npclist(int16_t choice);

        int32_t mapid;
        int8_t type;
        int8_t user_type;
        bool simple_mode;
        bool big_map;
        bool has_map;
        int16_t scale;
        nl::node map_node;
        nl::node minimap_node;
        nl::node marker_node;
        Texture map_sprite;
        Animation player_marker;
        int16_t combined_text_width;
        int16_t middle_right_x;
        int16_t bt_min_width;
        int16_t bt_max_width;
        int16_t bt_map_width;
        std::vector<Sprite> min_sprites;
        std::vector<Sprite> normal_sprites;
        std::vector<Sprite> max_sprites;
        std::vector<std::pair<std::string, Point<int16_t>>> static_marker_info;
        int16_t map_draw_origin_x;
        int16_t map_draw_origin_y;
        Point<int16_t> center_offset;
        Point<int16_t> min_dimensions;
        Point<int16_t> normal_dimensions;
        Point<int16_t> max_dimensions;
        Text combined_text;
        Text region_text;
        Text town_text;

        bool list_npc_enabled;
        nl::node list_npc_node;
        nl::node map_helper_node;
        std::vector<Sprite> list_npc_sprites;
        std::vector<MapObject*> list_npc_list;
        std::vector<Text> list_npc_names;
        std::vector<std::string> list_npc_full_names;
        Point<int16_t> list_npc_dimensions;
        Slider list_npc_slider;
        int16_t list_npc_offset;
        int16_t selected;
        Animation selected_marker;

        const CharStats& stats;
    };
}
