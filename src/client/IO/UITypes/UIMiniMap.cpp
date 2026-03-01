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
#include "UIMiniMap.h"

#include "UIWorldMap.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/ScrollingNotice.h"

#include "../../Gameplay/MapleMap/Npc.h"

#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    namespace
    {
        void truncate_text(Text& label, int16_t max_width)
        {
            if (label.empty() || max_width <= 0 || label.width() <= max_width)
            {
                return;
            }

            std::string text = label.get_text();
            while (!text.empty() && label.width() > max_width)
            {
                text.pop_back();
                label.change_text(text + "..");
            }
        }
    }

    UIMiniMap::UIMiniMap(const CharStats& stats)
        : UIDragElement<PosMINIMAP>(Point<int16_t>(128, 20)),
          mapid(-1),
          type(Setting<MiniMapType>::get().load()),
          user_type(type),
          simple_mode(Setting<MiniMapSimpleMode>::get().load()),
          big_map(true),
          has_map(false),
          scale(1),
          combined_text(Text::A12M, Text::LEFT, Text::WHITE),
          region_text(Text::A12B, Text::LEFT, Text::WHITE),
          town_text(Text::A12B, Text::LEFT, Text::WHITE),
          list_npc_enabled(false),
          list_npc_dimensions(150, 170),
          list_npc_offset(0),
          selected(-1),
          stats(stats)
    {
        if (position.y() < ScrollingNotice::HEIGHT)
        {
            position.set_y(ScrollingNotice::HEIGHT);
        }

        nl::node ui_window = nl::nx::ui["UIWindow2.img"];

        minimap_node = ui_window["MiniMap"];
        list_npc_node = minimap_node["ListNpc"];
        map_helper_node = nl::nx::map["MapHelper.img"];

        marker_node = Setting<MiniMapDefaultHelpers>::get().load() && ui_window["MiniMapSimpleMode"] ?
            ui_window["MiniMapSimpleMode"]["DefaultHelper"] :
            map_helper_node["minimap"];

        buttons[BT_MIN] = std::make_unique<MapleButton>(minimap_node["BtMin"], Point<int16_t>(195, -6));
        buttons[BT_MAX] = std::make_unique<MapleButton>(minimap_node["BtMax"], Point<int16_t>(209, -6));
        buttons[BT_SMALL] = std::make_unique<MapleButton>(minimap_node["BtSmall"], Point<int16_t>(223, -6));
        buttons[BT_BIG] = std::make_unique<MapleButton>(minimap_node["BtBig"], Point<int16_t>(223, -6));
        buttons[BT_MAP] = std::make_unique<MapleButton>(minimap_node["BtMap"], Point<int16_t>(237, -6));
        buttons[BT_NPC] = std::make_unique<MapleButton>(minimap_node["BtNpc"], Point<int16_t>(276, -6));

        player_marker = Animation(marker_node["user"]);
        selected_marker = Animation(minimap_node["iconNpc"]);
        map_draw_origin_x = 0;
        map_draw_origin_y = 0;
        center_offset = {};

        update_buttons();
        toggle_buttons();
    }

    void UIMiniMap::draw(float alpha) const
    {
        if (type == MIN)
        {
            for (const Sprite& sprite : min_sprites)
            {
                sprite.draw(position, alpha);
            }

            combined_text.draw(position + Point<int16_t>(7, -3));
        }
        else if (type == NORMAL)
        {
            for (const Sprite& sprite : normal_sprites)
            {
                sprite.draw(position, alpha);
            }

            if (has_map)
            {
                Animation portal_marker(marker_node["portal"]);

                for (const auto& portal : static_marker_info)
                {
                    portal_marker.draw(position + portal.second, alpha);
                }

                draw_movable_markers(position, alpha);

                if (list_npc_enabled)
                {
                    draw_npclist(normal_dimensions, alpha);
                }
            }
        }
        else
        {
            for (const Sprite& sprite : max_sprites)
            {
                sprite.draw(position, alpha);
            }

            region_text.draw(position + Point<int16_t>(48, 14));
            town_text.draw(position + Point<int16_t>(48, 28));

            if (has_map)
            {
                Animation portal_marker(marker_node["portal"]);

                for (const auto& portal : static_marker_info)
                {
                    portal_marker.draw(position + portal.second + Point<int16_t>(0, MAX_ADJ), alpha);
                }

                draw_movable_markers(position + Point<int16_t>(0, MAX_ADJ), alpha);

                if (list_npc_enabled)
                {
                    draw_npclist(max_dimensions, alpha);
                }
            }
        }

        UIElement::draw(alpha);
    }

    void UIMiniMap::update()
    {
        int32_t current_mapid = Stage::get().get_mapid();

        if (current_mapid != mapid)
        {
            mapid = current_mapid;
            map_node = NxHelper::Map::get_map_node_name(mapid);

            nl::node town = map_node["info"]["town"];
            nl::node mini_map = map_node["miniMap"];
            has_map = static_cast<bool>(mini_map);

            if (!has_map)
            {
                type = MIN;
            }
            else if (town && town.get_bool())
            {
                type = MAX;
            }
            else
            {
                type = user_type;
            }

            int32_t mag = mini_map ? static_cast<int32_t>(mini_map["mag"]) : 0;
            scale = 1 << std::max(0, mag);
            center_offset = mini_map ? Point<int16_t>(mini_map["centerX"], mini_map["centerY"]) : Point<int16_t>();

            update_text();
            update_buttons();
            update_canvas();
            update_static_markers();
            toggle_buttons();
            update_npclist();
        }

        for (Sprite& sprite : min_sprites)
        {
            sprite.update();
        }

        for (Sprite& sprite : normal_sprites)
        {
            sprite.update();
        }

        for (Sprite& sprite : max_sprites)
        {
            sprite.update();
        }

        for (Sprite& sprite : list_npc_sprites)
        {
            sprite.update();
        }

        if (selected >= 0)
        {
            selected_marker.update();
        }

        player_marker.update();
        UIElement::update();
    }

    bool UIMiniMap::remove_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        bool removed = UIDragElement::remove_cursor(clicked, cursorpos);
        bool slider_removed = list_npc_slider.remove_cursor(clicked);

        if (!is_in_range(cursorpos))
        {
            UI::get().clear_tooltip(Tooltip::MINIMAP);
        }

        return removed || slider_removed;
    }

    UIElement::CursorResult UIMiniMap::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (dragged)
        {
            return UIDragElement::send_cursor(clicked, cursorpos);
        }

        Point<int16_t> relative = cursorpos - position;

        if (list_npc_slider.isenabled())
        {
            Cursor::State slider_state = list_npc_slider.send_cursor(relative, clicked);
            if (slider_state != Cursor::IDLE)
            {
                return { slider_state, true };
            }
        }

        if (list_npc_enabled)
        {
            Point<int16_t> list_origin = Point<int16_t>(
                10 + (type == MAX ? max_dimensions : normal_dimensions).x(),
                23
            );
            Rectangle<int16_t> list_bounds(
                list_origin,
                list_origin + Point<int16_t>(LISTNPC_ITEM_WIDTH, LISTNPC_ITEM_HEIGHT * 8)
            );

            if (list_bounds.contains(relative))
            {
                int16_t list_index = list_npc_offset + (relative.y() - list_origin.y()) / LISTNPC_ITEM_HEIGHT;
                bool in_list = list_index >= 0 && list_index < static_cast<int16_t>(list_npc_names.size());

                if (clicked)
                {
                    select_npclist(in_list ? list_index : -1);
                }
                else if (in_list)
                {
                    UI::get().show_text(Tooltip::MINIMAP, list_npc_full_names[list_index]);
                }

                return { Cursor::IDLE, true };
            }
        }

        bool found = false;
        MapObjects* npcs = Stage::get().get_npcs().get_npcs();

        for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
        {
            Point<int16_t> npc_pos =
                (npc->second->get_position() + center_offset) / scale +
                Point<int16_t>(map_draw_origin_x, map_draw_origin_y);
            Rectangle<int16_t> marker_spot(npc_pos - Point<int16_t>(4, 8), npc_pos + Point<int16_t>(4, 8));

            if (type == MAX)
            {
                marker_spot.shift(Point<int16_t>(0, MAX_ADJ));
            }

            if (marker_spot.contains(relative))
            {
                found = true;

                Npc* map_npc = static_cast<Npc*>(npc->second.get());
                UI::get().show_map(
                    Tooltip::MINIMAP,
                    map_npc->get_name(),
                    map_npc->get_func(),
                    0,
                    false,
                    false
                );
                break;
            }
        }

        if (!found)
        {
            for (const auto& portal : static_marker_info)
            {
                Rectangle<int16_t> marker_spot(portal.second, portal.second + Point<int16_t>(8, 8));

                if (type == MAX)
                {
                    marker_spot.shift(Point<int16_t>(0, MAX_ADJ));
                }

                if (marker_spot.contains(relative))
                {
                    int32_t target_mapid = map_node["portal"][portal.first]["tm"];
                    std::string portal_category = NxHelper::Map::get_map_category(target_mapid);
                    std::string target_map_str = std::to_string(target_mapid);
                    std::string target_name = nl::nx::string["Map.img"][portal_category][target_map_str]["mapName"];

                    if (!target_name.empty())
                    {
                        UI::get().show_map(Tooltip::MINIMAP, target_name, "", target_mapid, false, true);
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!found)
        {
            UI::get().clear_tooltip(Tooltip::MINIMAP);
        }

        return UIDragElement::send_cursor(clicked, cursorpos);
    }

    void UIMiniMap::send_scroll(double yoffset)
    {
        if (list_npc_enabled && list_npc_slider.isenabled())
        {
            list_npc_slider.send_scroll(yoffset);
        }
    }

    void UIMiniMap::send_key(int32_t, bool pressed, bool)
    {
        if (!pressed || !has_map)
        {
            return;
        }

        type = type < MAX ? type + 1 : MIN;
        user_type = type;
        Setting<MiniMapType>::get().save(static_cast<uint8_t>(user_type));
        toggle_buttons();
    }

    Button::State UIMiniMap::button_pressed(uint16_t buttonid)
    {
        switch (buttonid)
        {
        case BT_MIN:
            if (type > MIN)
            {
                type -= 1;
                user_type = type;
                Setting<MiniMapType>::get().save(static_cast<uint8_t>(user_type));
                toggle_buttons();
            }
            return type == MIN ? Button::DISABLED : Button::NORMAL;
        case BT_MAX:
            if (has_map && type < MAX)
            {
                type += 1;
                user_type = type;
                Setting<MiniMapType>::get().save(static_cast<uint8_t>(user_type));
                toggle_buttons();
            }
            return type == MAX ? Button::DISABLED : Button::NORMAL;
        case BT_SMALL:
        case BT_BIG:
            big_map = !big_map;
            toggle_buttons();
            break;
        case BT_MAP:
            UI::get().emplace<UIWorldMap>();
            break;
        case BT_NPC:
            set_npclist_active(!list_npc_enabled);
            break;
        }

        return Button::NORMAL;
    }

    void UIMiniMap::update_buttons()
    {
        bt_min_width = buttons[BT_MIN]->width() + 1;
        bt_max_width = buttons[BT_MAX]->width() + 1;
        bt_map_width = buttons[BT_MAP]->width() + 1;
        combined_text_width = combined_text.width();
    }

    void UIMiniMap::toggle_buttons()
    {
        int16_t bt_min_x = 0;

        if (type == MIN)
        {
            buttons[BT_MAP]->set_active(true);
            buttons[BT_MAX]->set_active(true);
            buttons[BT_MIN]->set_active(true);
            buttons[BT_NPC]->set_active(false);
            buttons[BT_SMALL]->set_active(false);
            buttons[BT_BIG]->set_active(false);

            buttons[BT_MIN]->set_state(Button::DISABLED);
            buttons[BT_MAX]->set_state(has_map ? Button::NORMAL : Button::DISABLED);

            bt_min_x = combined_text_width + 11;

            buttons[BT_MIN]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            bt_min_x += bt_min_width;
            buttons[BT_MAX]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            bt_min_x += bt_max_width;
            buttons[BT_MAP]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

            min_dimensions = Point<int16_t>(bt_min_x + bt_map_width + 7, 20);

            update_dimensions();
            dragarea = dimension;
            set_npclist_active(false);
        }
        else
        {
            bool has_npcs = Stage::get().get_npcs().get_npcs()->size() > 0;

            buttons[BT_MAP]->set_active(true);
            buttons[BT_MAX]->set_active(true);
            buttons[BT_MIN]->set_active(true);
            buttons[BT_NPC]->set_active(has_npcs);

            buttons[BT_BIG]->set_active(!big_map);
            buttons[BT_SMALL]->set_active(big_map);
            buttons[BT_MIN]->set_state(Button::NORMAL);

            bt_min_x = middle_right_x - (
                bt_min_width +
                buttons[BT_SMALL]->width() + 1 +
                bt_max_width +
                bt_map_width +
                (has_npcs ? buttons[BT_NPC]->width() : 0)
            );

            buttons[BT_MIN]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            bt_min_x += bt_max_width;
            buttons[BT_MAX]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            bt_min_x += bt_max_width;
            buttons[BT_SMALL]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            buttons[BT_BIG]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            bt_min_x += bt_max_width;
            buttons[BT_MAP]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
            bt_min_x += bt_map_width;
            buttons[BT_NPC]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

            buttons[BT_MAX]->set_state(type == MAX ? Button::DISABLED : Button::NORMAL);
            set_npclist_active(list_npc_enabled && has_npcs);
            dragarea = Point<int16_t>(dimension.x(), 20);
        }
    }

    void UIMiniMap::update_text()
    {
        NxHelper::Map::MapInfo map_info = NxHelper::Map::get_map_info_by_id(mapid);
        combined_text.change_text(map_info.full_name);
        region_text.change_text(map_info.name);
        town_text.change_text(map_info.street_name);
    }

    void UIMiniMap::update_canvas()
    {
        min_sprites.clear();
        normal_sprites.clear();
        max_sprites.clear();

        nl::node min_src = minimap_node["Min"];
        nl::node normal_src = minimap_node["MinMap"];
        nl::node max_src = minimap_node["MaxMap"];
        nl::node middle_center = minimap_node["MaxMap"]["c"];

        map_sprite = Texture(map_node["miniMap"]["canvas"]);
        Point<int16_t> map_dimensions = map_sprite.get_dimensions();

        int16_t mark_text_width = 48 + std::max(region_text.width(), town_text.width()) + 10;
        int16_t window_width = std::max<int16_t>(178, std::max<int16_t>(mark_text_width, map_dimensions.x() + 20));
        int16_t c_stretch = std::max<int16_t>(0, window_width - 128);
        int16_t ur_x_offset = CENTER_START_X + c_stretch;

        map_draw_origin_x = std::max<int16_t>(10, window_width / 2 - map_dimensions.x() / 2);

        int16_t m_stretch = 5;
        int16_t down_y_offset = 22;
        if (map_dimensions.y() > 20)
        {
            m_stretch = map_dimensions.y() - 17;
            down_y_offset = 17 + m_stretch;
            map_draw_origin_y = 20;
        }
        else
        {
            m_stretch = 5;
            down_y_offset = 17 + m_stretch;
            map_draw_origin_y = 10 + m_stretch - map_dimensions.y();
        }

        middle_right_x = ur_x_offset + 55;

        int16_t min_c_stretch = combined_text_width + 18 + bt_min_width + bt_max_width + bt_map_width - 128;

        min_sprites.emplace_back(min_src["c"], DrawArgument(Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(min_c_stretch, 0)));
        min_sprites.emplace_back(min_src["w"], DrawArgument(Point<int16_t>(0, 0)));
        min_sprites.emplace_back(min_src["e"], DrawArgument(Point<int16_t>(min_c_stretch + CENTER_START_X, 0)));

        normal_sprites.emplace_back(middle_center, DrawArgument(Point<int16_t>(7, 10), Point<int16_t>(c_stretch + 114, m_stretch + 27)));
        if (has_map)
        {
            normal_sprites.emplace_back(map_node["miniMap"]["canvas"], DrawArgument(Point<int16_t>(map_draw_origin_x, map_draw_origin_y)));
        }

        normal_sprites.emplace_back(normal_src["w"], DrawArgument(Point<int16_t>(0, ML_MR_Y), Point<int16_t>(0, m_stretch)));
        normal_sprites.emplace_back(normal_src["e"], DrawArgument(Point<int16_t>(middle_right_x, ML_MR_Y), Point<int16_t>(0, m_stretch)));
        normal_sprites.emplace_back(normal_src["n"], DrawArgument(Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(c_stretch, 0)));
        normal_sprites.emplace_back(normal_src["nw"], DrawArgument(Point<int16_t>(0, 0)));
        normal_sprites.emplace_back(normal_src["ne"], DrawArgument(Point<int16_t>(ur_x_offset, 0)));
        normal_sprites.emplace_back(normal_src["s"], DrawArgument(Point<int16_t>(CENTER_START_X, down_y_offset + 18), Point<int16_t>(c_stretch, 0)));
        normal_sprites.emplace_back(normal_src["sw"], DrawArgument(Point<int16_t>(0, down_y_offset)));
        normal_sprites.emplace_back(normal_src["se"], DrawArgument(Point<int16_t>(ur_x_offset, down_y_offset)));

        normal_dimensions = Point<int16_t>(ur_x_offset + 64, down_y_offset + 27);

        max_sprites.emplace_back(middle_center, DrawArgument(Point<int16_t>(7, 50), Point<int16_t>(c_stretch + 114, m_stretch + 27)));
        if (has_map)
        {
            max_sprites.emplace_back(map_node["miniMap"]["canvas"], DrawArgument(Point<int16_t>(map_draw_origin_x, map_draw_origin_y + MAX_ADJ)));
        }

        max_sprites.emplace_back(max_src["w"], DrawArgument(Point<int16_t>(0, ML_MR_Y + MAX_ADJ), Point<int16_t>(0, m_stretch)));
        max_sprites.emplace_back(max_src["e"], DrawArgument(Point<int16_t>(middle_right_x, ML_MR_Y + MAX_ADJ), Point<int16_t>(0, m_stretch)));
        max_sprites.emplace_back(max_src["n"], DrawArgument(Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(c_stretch, 0)));
        max_sprites.emplace_back(max_src["nw"], DrawArgument(Point<int16_t>(0, 0)));
        max_sprites.emplace_back(max_src["ne"], DrawArgument(Point<int16_t>(ur_x_offset, 0)));
        max_sprites.emplace_back(max_src["s"], DrawArgument(Point<int16_t>(CENTER_START_X, down_y_offset + MAX_ADJ + 18), Point<int16_t>(c_stretch, 0)));
        max_sprites.emplace_back(max_src["sw"], DrawArgument(Point<int16_t>(0, down_y_offset + MAX_ADJ)));
        max_sprites.emplace_back(max_src["se"], DrawArgument(Point<int16_t>(ur_x_offset, down_y_offset + MAX_ADJ)));
        max_sprites.emplace_back(map_helper_node["mark"][map_node["info"]["mapMark"]], DrawArgument(Point<int16_t>(7, 17)));

        max_dimensions = normal_dimensions + Point<int16_t>(0, MAX_ADJ);
        update_dimensions();
    }

    void UIMiniMap::draw_movable_markers(Point<int16_t> init_pos, float alpha) const
    {
        if (!has_map)
        {
            return;
        }

        Animation npc_marker(marker_node["npc"]);
        Point<int16_t> npc_offset = npc_marker.get_dimensions() / Point<int16_t>(2, 1);

        MapObjects* npcs = Stage::get().get_npcs().get_npcs();
        for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
        {
            Point<int16_t> npc_pos = npc->second->get_position();
            npc_marker.draw((npc_pos + center_offset) / scale - npc_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos, alpha);
        }

        Animation other_marker(marker_node["another"]);
        Point<int16_t> other_offset = other_marker.get_dimensions() / Point<int16_t>(2, 1);

        MapObjects* chars = Stage::get().get_chars().get_chars();
        for (auto chr = chars->begin(); chr != chars->end(); ++chr)
        {
            Point<int16_t> chr_pos = chr->second->get_position();
            other_marker.draw((chr_pos + center_offset) / scale - other_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos, alpha);
        }

        Point<int16_t> player_pos = Stage::get().get_player().get_position();
        Point<int16_t> player_offset = player_marker.get_dimensions() / Point<int16_t>(2, 1);
        player_marker.draw((player_pos + center_offset) / scale - player_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos, alpha);
    }

    void UIMiniMap::update_static_markers()
    {
        static_marker_info.clear();

        if (!has_map)
        {
            return;
        }

        Animation portal_marker(marker_node["portal"]);
        Point<int16_t> marker_offset = portal_marker.get_dimensions() / Point<int16_t>(2, 1);

        for (nl::node portal : map_node["portal"])
        {
            int32_t portal_type = portal["pt"];
            if (portal_type == 2)
            {
                Point<int16_t> marker_pos =
                    (Point<int16_t>(portal["x"], portal["y"]) + center_offset) / scale -
                    marker_offset +
                    Point<int16_t>(map_draw_origin_x, map_draw_origin_y);
                static_marker_info.emplace_back(portal.name(), marker_pos);
            }
        }
    }

    void UIMiniMap::set_npclist_active(bool active)
    {
        list_npc_enabled = active;

        if (!active)
        {
            select_npclist(-1);
        }

        update_dimensions();
    }

    void UIMiniMap::update_dimensions()
    {
        if (type == MIN)
        {
            dimension = min_dimensions;
            return;
        }

        Point<int16_t> base = type == MAX ? max_dimensions : normal_dimensions;
        dimension = base;

        if (list_npc_enabled)
        {
            dimension += list_npc_dimensions;
            dimension.set_y(std::max<int16_t>(base.y(), list_npc_dimensions.y()));
        }
    }

    void UIMiniMap::update_npclist()
    {
        list_npc_sprites.clear();
        list_npc_names.clear();
        list_npc_full_names.clear();
        list_npc_list.clear();
        selected = -1;
        list_npc_offset = 0;

        MapObjects* npcs = Stage::get().get_npcs().get_npcs();

        for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
        {
            list_npc_list.emplace_back(npc->second.get());

            Npc* map_npc = static_cast<Npc*>(npc->second.get());
            std::string name = map_npc->get_name();
            std::string func = map_npc->get_func();

            if (!func.empty())
            {
                name += " (" + func + ")";
            }

            Text name_text(Text::A11M, Text::LEFT, Text::WHITE, name);
            truncate_text(name_text, LISTNPC_TEXT_WIDTH - (npcs->size() > 8 ? 0 : 20));

            list_npc_names.emplace_back(name_text);
            list_npc_full_names.emplace_back(name);
        }

        const Point<int16_t> list_pos(type == MAX ? max_dimensions.x() : normal_dimensions.x(), 0);
        int16_t c_stretch = 20;
        int16_t m_stretch = 102;

        if (list_npc_names.size() > 8)
        {
            list_npc_slider = Slider(
                0,
                Range<int16_t>(23, 11 + LISTNPC_ITEM_HEIGHT * 8),
                list_pos.x() + LISTNPC_ITEM_WIDTH + 1,
                8,
                static_cast<int16_t>(list_npc_names.size()),
                [&](bool upwards)
                {
                    int16_t shift = upwards ? -1 : 1;
                    bool above = list_npc_offset + shift >= 0;
                    bool below = list_npc_offset + 8 + shift <= static_cast<int16_t>(list_npc_names.size());

                    if (above && below)
                    {
                        list_npc_offset += shift;
                    }
                }
            );

            c_stretch += 12;
        }
        else
        {
            list_npc_slider.setenabled(false);
            m_stretch = std::max<int16_t>(0, LISTNPC_ITEM_HEIGHT * static_cast<int16_t>(list_npc_names.size()) - 34);
            c_stretch -= 17;
        }

        list_npc_sprites.emplace_back(list_npc_node["c"], DrawArgument(list_pos + Point<int16_t>(CENTER_START_X, M_START), Point<int16_t>(c_stretch, m_stretch)));
        list_npc_sprites.emplace_back(list_npc_node["w"], DrawArgument(list_pos + Point<int16_t>(0, M_START), Point<int16_t>(0, m_stretch)));
        list_npc_sprites.emplace_back(list_npc_node["e"], DrawArgument(list_pos + Point<int16_t>(CENTER_START_X + c_stretch, M_START), Point<int16_t>(0, m_stretch)));
        list_npc_sprites.emplace_back(list_npc_node["n"], DrawArgument(list_pos + Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(c_stretch, 0)));
        list_npc_sprites.emplace_back(list_npc_node["s"], DrawArgument(list_pos + Point<int16_t>(CENTER_START_X, M_START + m_stretch), Point<int16_t>(c_stretch, 0)));
        list_npc_sprites.emplace_back(list_npc_node["nw"], DrawArgument(list_pos + Point<int16_t>(0, 0)));
        list_npc_sprites.emplace_back(list_npc_node["ne"], DrawArgument(list_pos + Point<int16_t>(CENTER_START_X + c_stretch, 0)));
        list_npc_sprites.emplace_back(list_npc_node["sw"], DrawArgument(list_pos + Point<int16_t>(0, M_START + m_stretch)));
        list_npc_sprites.emplace_back(list_npc_node["se"], DrawArgument(list_pos + Point<int16_t>(CENTER_START_X + c_stretch, M_START + m_stretch)));

        list_npc_dimensions = Point<int16_t>(CENTER_START_X * 2 + c_stretch, M_START + m_stretch + 30);
        update_dimensions();
    }

    void UIMiniMap::draw_npclist(Point<int16_t> minimap_dims, float alpha) const
    {
        Animation npc_marker(marker_node["npc"]);

        for (const Sprite& sprite : list_npc_sprites)
        {
            sprite.draw(position, alpha);
        }

        Point<int16_t> list_pos = position + Point<int16_t>(minimap_dims.x() + 10, 23);

        for (int16_t i = 0; i + list_npc_offset < static_cast<int16_t>(list_npc_list.size()) && i < 8; ++i)
        {
            if (selected - list_npc_offset == i)
            {
                ColorBox highlight(
                    LISTNPC_ITEM_WIDTH - (list_npc_slider.isenabled() ? 0 : 30),
                    LISTNPC_ITEM_HEIGHT,
                    Geometry::WHITE,
                    0.35f
                );
                highlight.draw(DrawArgument(list_pos));
            }

            npc_marker.draw(DrawArgument(list_pos + Point<int16_t>(0, 2)), alpha);
            list_npc_names[list_npc_offset + i].draw(list_pos + Point<int16_t>(14, -2));
            list_pos.shift_y(LISTNPC_ITEM_HEIGHT);
        }

        if (list_npc_slider.isenabled())
        {
            list_npc_slider.draw(position);
        }

        if (selected >= 0 && selected < static_cast<int16_t>(list_npc_list.size()))
        {
            Point<int16_t> npc_pos =
                (list_npc_list[selected]->get_position() + center_offset) / scale +
                Point<int16_t>(map_draw_origin_x, map_draw_origin_y - selected_marker.get_dimensions().y() + (type == MAX ? MAX_ADJ : 0));

            selected_marker.draw(position + npc_pos, 0.5f);
        }
    }

    void UIMiniMap::select_npclist(int16_t choice)
    {
        if (selected >= 0 && selected < static_cast<int16_t>(list_npc_names.size()))
        {
            list_npc_names[selected].change_color(Text::WHITE);
        }

        if (choice < 0 || choice >= static_cast<int16_t>(list_npc_names.size()))
        {
            selected = -1;
            return;
        }

        selected = choice;
        list_npc_names[selected].change_color(Text::BLACK);
    }
}
