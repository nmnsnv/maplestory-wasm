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
#include "../Console.h"
#include "../Graphics/Text.h"

#include "nlnx/node.hpp"

#ifdef MS_PLATFORM_WASM
#include <emscripten.h>
#endif

#include <cstdint>
#include <string>
#include <unordered_map>

namespace jrc
{
    namespace string_conversion
    {
        template<typename T>
        inline T or_default(const std::string& str, T def)
        {
            try
            {
                int32_t intval = std::stoi(str);
                return static_cast<T>(intval);
            }
            catch (const std::exception& ex)
            {
                Console::get().print(__func__, ex);
                return def;
            }
        }

        template<typename T>
        inline T or_zero(const std::string& str)
        {
            return or_default<T>(str, T(0));
        }
    };

    namespace string_format
    {
        // Format a number string so that each 3 decimal points
        // are seperated by a ',' character.
        void split_number(std::string& input);

        // Prefix an id with zeroes so that it has the minimum specified length.
        std::string extend_id(int32_t id, size_t length);
    };

    namespace bytecode
    {
        // Check if a bit mask contains the specified value.
        bool compare(int32_t mask, int32_t value);
    }

    namespace NxHelper
    {
        namespace Map
        {
            struct MapInfo
            {
                std::string description;
                std::string name;
                std::string street_name;
                std::string full_name;
            };

            MapInfo get_map_info_by_id(int32_t mapid);
            std::string get_map_category(int32_t mapid);
            std::unordered_map<int64_t, std::pair<std::string, std::string>> get_life_on_map(int32_t mapid);
            nl::node get_map_node_name(int32_t mapid);
        }
    }

#ifdef MS_PLATFORM_WASM
    inline std::string getBrowserHostname()
    {
        // Pass a dummy variadic argument to avoid pedantic zero-arg variadic macro warnings.
        char* hostname = (char*)EM_ASM_PTR({
            var hostname = window.location.hostname;
            var len = lengthBytesUTF8(hostname) + 1;
            var ptr = _malloc(len);
            stringToUTF8(hostname, ptr, len);
            return ptr;
        }, 0);
        std::string result;
        if (hostname && strlen(hostname) > 0) {
            result = hostname;
        } else {
            result = "127.0.0.1";
        }
        if (hostname) free(hostname);
        return result;
    }
#else
    inline std::string getBrowserHostname()
    {
        return "127.0.0.1";
    }
#endif
}
