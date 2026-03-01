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
#include "UIWorldMap.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"
#include "../../Util/Misc.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    UIWorldMap::UIWorldMap()
        : UIDragElement<PosMAP>(Point<int16_t>()),
          search(true),
          show_path_img(false),
          mapid(-1),
          search_text_dim(82, 14)
    {
        nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
        nl::node world_map = nl::nx::ui["UIWindow2.img"]["WorldMap"];
        nl::node world_map_search = world_map["WorldMapSearch"];
        nl::node border = world_map["Border"]["0"];
        nl::node background = world_map_search["backgrnd"];
        nl::node helper = nl::nx::map["MapHelper.img"]["worldMap"];

        sprites.emplace_back(border);

        cur_pos = helper["curPos"];
        for (size_t i = 0; i < MAPSPOT_TYPE_MAX; ++i)
        {
            npc_pos[i] = helper["npcPos" + std::to_string(i)];
        }

        search_background = background;
        search_notice = world_map_search["notice"];

        bg_dimensions = Texture(border).get_dimensions();
        bg_search_dimensions = search_background.get_dimensions();
        background_dimensions = Point<int16_t>(bg_dimensions.x(), 0);
        base_position = Point<int16_t>(bg_dimensions.x() / 2, bg_dimensions.y() / 2 + 15);

        Point<int16_t> close_pos(bg_dimensions.x() - 22, 4);
        buttons[BT_CLOSE] = std::make_unique<MapleButton>(close, close_pos);
        buttons[BT_SEARCH] = std::make_unique<MapleButton>(world_map["BtSearch"]);
        buttons[BT_AUTOFLY] = std::make_unique<MapleButton>(world_map["BtAutoFly_1"]);
        buttons[BT_NAVIREG] = std::make_unique<MapleButton>(world_map["BtNaviRegister"]);
        buttons[BT_SEARCH_CLOSE] = std::make_unique<MapleButton>(close, close_pos + Point<int16_t>(bg_search_dimensions.x(), 0));
        buttons[BT_ALLSEARCH] = std::make_unique<MapleButton>(world_map_search["BtAllsearch"], background_dimensions);

        Point<int16_t> search_pos = background_dimensions + Point<int16_t>(13, 25);
        search_text = Textfield(
            Text::A11M,
            Text::LEFT,
            Text::BLACK,
            Rectangle<int16_t>(search_pos, search_pos + search_text_dim),
            8
        );

        set_search(true);
        dragarea = Point<int16_t>(bg_dimensions.x(), 20);
    }

    void UIWorldMap::draw(float alpha) const
    {
        UIElement::draw_sprites(alpha);

        if (search)
        {
            search_background.draw(position + background_dimensions);
            search_notice.draw(position + background_dimensions);
            search_text.draw(position);
        }

        base_img.draw(position + base_position);

        for (const auto& entry : buttons)
        {
            if (entry.first >= BT_LINK0 && entry.second && entry.second->get_state() == Button::MOUSEOVER)
            {
                auto image = link_images.find(entry.first);
                if (image != link_images.end())
                {
                    image->second.draw(position + base_position);
                    break;
                }
            }
        }

        if (show_path_img)
        {
            path_img.draw(position + base_position);
        }

        bool found = false;
        for (const auto& spot : map_spots)
        {
            spot.second.marker.draw(position + base_position + spot.first);

            if (found)
            {
                continue;
            }

            for (int32_t spot_mapid : spot.second.map_ids)
            {
                if (spot_mapid == mapid)
                {
                    found = true;
                    if (spot.second.type < MAPSPOT_TYPE_MAX)
                    {
                        npc_pos[spot.second.type].draw(position + base_position + spot.first, alpha);
                    }
                    cur_pos.draw(position + base_position + spot.first, alpha);
                    break;
                }
            }
        }

        UIElement::draw_buttons(alpha);
    }

    void UIWorldMap::update()
    {
        int32_t current_mapid = Stage::get().get_mapid();
        if (current_mapid != mapid)
        {
            mapid = current_mapid;

            int32_t prefix = mapid / 10000000;
            user_map = "WorldMap0" + std::to_string(prefix);
            update_world(user_map);
        }

        search_text.update(position);

        for (size_t i = 0; i < MAPSPOT_TYPE_MAX; ++i)
        {
            npc_pos[i].update();
        }

        cur_pos.update();
        UIElement::update();
    }

    void UIWorldMap::toggle_active()
    {
        bool was_active = active;
        UIElement::toggle_active();

        if (was_active && !active && search_text.get_state() == Textfield::FOCUSED)
        {
            UI::get().focus_textfield(nullptr);
        }

        if (!active)
        {
            set_search(true);
            update_world(user_map);
        }
    }

    bool UIWorldMap::remove_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        bool removed = UIDragElement::remove_cursor(clicked, cursorpos);

        if (!is_in_range(cursorpos))
        {
            UI::get().clear_tooltip(Tooltip::WORLDMAP);
            show_path_img = false;
        }

        return removed;
    }

    UIElement::CursorResult UIWorldMap::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (dragged)
        {
            return UIDragElement::send_cursor(clicked, cursorpos);
        }

        Cursor::State text_state = search_text.send_cursor(cursorpos, clicked);
        if (text_state != Cursor::IDLE)
        {
            return { text_state, true };
        }

        show_path_img = false;

        for (const auto& path : map_spots)
        {
            Point<int16_t> marker_lt = path.first + position + base_position - Point<int16_t>(10, 10);
            Point<int16_t> marker_rb = marker_lt + path.second.marker.get_dimensions();
            Rectangle<int16_t> bounds(marker_lt, marker_rb);

            if (!clicked && bounds.contains(cursorpos))
            {
                path_img = path.second.path;
                show_path_img = path_img.is_valid();

                UI::get().show_map(
                    Tooltip::WORLDMAP,
                    path.second.title,
                    path.second.description,
                    path.second.map_ids.empty() ? 0 : path.second.map_ids.front(),
                    path.second.bolded,
                    false
                );
                break;
            }
        }

        return UIDragElement::send_cursor(clicked, cursorpos);
    }

    void UIWorldMap::send_key(int32_t, bool pressed, bool escape)
    {
        if (!pressed || !escape)
        {
            return;
        }

        if (search)
        {
            set_search(false);
        }
        else if (parent_map.empty())
        {
            toggle_active();
        }
        else
        {
            update_world(parent_map);
        }
    }

    Button::State UIWorldMap::button_pressed(uint16_t buttonid)
    {
        if (buttonid >= BT_LINK0)
        {
            auto iter = link_maps.find(buttonid);
            if (iter != link_maps.end())
            {
                update_world(iter->second);
            }
            return Button::IDENTITY;
        }

        switch (buttonid)
        {
        case BT_CLOSE:
            toggle_active();
            return Button::NORMAL;
        case BT_SEARCH:
            set_search(!search);
            return Button::NORMAL;
        case BT_SEARCH_CLOSE:
            set_search(false);
            return Button::NORMAL;
        default:
            return Button::DISABLED;
        }
    }

    void UIWorldMap::set_search(bool enable)
    {
        if (!enable && search_text.get_state() == Textfield::FOCUSED)
        {
            UI::get().focus_textfield(nullptr);
        }

        search = enable;
        buttons[BT_SEARCH_CLOSE]->set_active(enable);
        buttons[BT_ALLSEARCH]->set_active(enable);
        search_text.set_state(enable ? Textfield::NORMAL : Textfield::DISABLED);
        dimension = enable ? bg_dimensions + Point<int16_t>(bg_search_dimensions.x(), 0) : bg_dimensions;
    }

    void UIWorldMap::update_world(const std::string& map)
    {
        nl::node world_map = nl::nx::map["WorldMap"][map + ".img"];
        if (!world_map)
        {
            world_map = nl::nx::map["WorldMap"]["WorldMap.img"];
        }

        base_img = world_map["BaseImg"][0];
        parent_map = world_map["info"]["parentMap"].get_string();

        link_images.clear();
        link_maps.clear();

        for (auto& button : buttons)
        {
            if (button.first >= BT_LINK0 && button.second)
            {
                button.second->set_active(false);
            }
        }

        uint16_t link_id = BT_LINK0;
        for (nl::node link : world_map["MapLink"])
        {
            if (link_id > BT_LINK9)
            {
                break;
            }

            nl::node link_info = link["link"];
            Texture link_image = link_info["linkImg"];

            link_images[link_id] = link_image;
            link_maps[link_id] = link_info["linkMap"].get_string();
            buttons[link_id] = std::make_unique<AreaButton>(
                base_position - link_image.get_origin(),
                link_image.get_dimensions()
            );
            buttons[link_id]->set_active(true);
            ++link_id;
        }

        nl::node map_image = nl::nx::map["MapHelper.img"]["worldMap"]["mapImage"];
        map_spots.clear();

        for (nl::node list : world_map["MapList"])
        {
            std::string description = list["desc"];
            std::string title = list["title"];
            uint8_t type = static_cast<uint8_t>(static_cast<int32_t>(list["type"]));
            Texture marker = map_image[std::to_string(type)];

            std::vector<int32_t> map_ids;
            for (nl::node map_no : list["mapNo"])
            {
                map_ids.push_back(map_no);
            }

            bool bolded = !description.empty();

            if (description.empty() && title.empty() && !map_ids.empty())
            {
                NxHelper::Map::MapInfo map_info = NxHelper::Map::get_map_info_by_id(map_ids.front());
                description = map_info.description;
                title = map_info.full_name;
                bolded = !description.empty();
            }

            map_spots.emplace_back(
                Point<int16_t>(list["spot"]),
                MapSpot{
                    description,
                    Texture(list["path"]),
                    title,
                    type,
                    marker,
                    bolded,
                    map_ids
                }
            );
        }
    }

    Point<int16_t> UIWorldMap::get_search_pos() const
    {
        return position + background_dimensions + Point<int16_t>(13, 25);
    }
}
