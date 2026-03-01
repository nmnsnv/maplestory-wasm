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
#include "Misc.h"

#include "nlnx/nx.hpp"

#include <algorithm>

namespace jrc
{
    namespace string_format
    {
        void split_number(std::string& input)
        {
            for (size_t i = input.size(); i > 3; i -= 3)
            {
                input.insert(i - 3, 1, ',');
            }
        }

        std::string extend_id(int32_t id, size_t length)
        {
            std::string strid = std::to_string(id);
            if (strid.size() < length)
            {
                strid.insert(0, length - strid.size(), '0');
            }
            return strid;
        }
    }

    namespace bytecode
    {
        bool compare(int32_t mask, int32_t value)
        {
            return (mask & value) != 0;
        }
    }

    namespace NxHelper
    {
        namespace Map
        {
            namespace
            {
                std::string make_full_name(const std::string& street_name, const std::string& name)
                {
                    if (street_name.empty())
                    {
                        return name;
                    }

                    if (name.empty())
                    {
                        return street_name;
                    }

                    return street_name + " : " + name;
                }
            }

            MapInfo get_map_info_by_id(int32_t mapid)
            {
                std::string map_category = get_map_category(mapid);
                std::string mapid_str = std::to_string(mapid);
                nl::node map_info = nl::nx::string["Map.img"][map_category][mapid_str];

                std::string description = map_info["mapDesc"];
                std::string name = map_info["mapName"];
                std::string street_name = map_info["streetName"];

                return {
                    description,
                    name,
                    street_name,
                    make_full_name(street_name, name)
                };
            }

            std::string get_map_category(int32_t mapid)
            {
                if (mapid < 100000000)
                {
                    return "maple";
                }

                if (mapid < 200000000)
                {
                    return "victoria";
                }

                if (mapid < 300000000)
                {
                    return "ossyria";
                }

                if (mapid < 540000000)
                {
                    return "elin";
                }

                if (mapid < 600000000)
                {
                    return "singapore";
                }

                if (mapid < 670000000)
                {
                    return "MasteriaGL";
                }

                if (mapid < 682000000)
                {
                    int32_t prefix3 = (mapid / 1000000) * 1000000;
                    int32_t prefix4 = (mapid / 100000) * 100000;

                    if (prefix3 == 674000000 || prefix4 == 680100000 || prefix4 == 889100000)
                    {
                        return "etc";
                    }

                    if (prefix3 == 677000000)
                    {
                        return "Episode1GL";
                    }

                    return "weddingGL";
                }

                if (mapid < 683000000)
                {
                    return "HalloweenGL";
                }

                if (mapid < 800000000)
                {
                    return "event";
                }

                if (mapid < 900000000)
                {
                    return "jp";
                }

                return "etc";
            }

            std::unordered_map<int64_t, std::pair<std::string, std::string>> get_life_on_map(int32_t mapid)
            {
                std::unordered_map<int64_t, std::pair<std::string, std::string>> map_life;

                nl::node map_node = get_map_node_name(mapid);

                for (nl::node life : map_node["life"])
                {
                    int64_t life_id = life["id"];
                    std::string life_id_str = std::to_string(life_id);
                    std::string life_type = life["type"];
                    bool hide_life = life["hide"].get_bool();

                    if (hide_life)
                    {
                        continue;
                    }

                    if (life_type == "m")
                    {
                        nl::node life_name = nl::nx::string["Mob.img"][life_id_str]["name"];
                        std::string mob_id = string_format::extend_id(static_cast<int32_t>(life_id), 7);
                        nl::node life_level = nl::nx::mob[mob_id + ".img"]["info"]["level"];

                        if (life_name)
                        {
                            std::string label = life_name;
                            if (life_level)
                            {
                                label += " (Lv. " + std::to_string(static_cast<int32_t>(life_level)) + ")";
                            }
                            map_life[life_id] = { life_type, label };
                        }
                    }
                    else if (life_type == "n")
                    {
                        nl::node life_name = nl::nx::string["Npc.img"][life_id_str]["name"];
                        if (life_name)
                        {
                            map_life[life_id] = { life_type, static_cast<std::string>(life_name) };
                        }
                    }
                }

                return map_life;
            }

            nl::node get_map_node_name(int32_t mapid)
            {
                std::string prefix = std::to_string(mapid / 100000000);
                std::string mapid_str = string_format::extend_id(mapid, 9);

                return nl::nx::map["Map"]["Map" + prefix][mapid_str + ".img"];
            }
        }
    }
}
