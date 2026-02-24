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
#include "NxFiles.h"

#include "../Console.h"

#include "nlnx/nx.hpp"
#include "nlnx/node.hpp"

#include <fstream>

#ifdef MS_PLATFORM_WASM
#include "../LazyFS/LazyFS.h"
#endif

#include <unistd.h>
#include <cstdio>

namespace jrc
{
    Error NxFiles::init()
    {
        constexpr const char* REQUIRED_ROOT_NX = "Base.nx";
#ifdef MS_PLATFORM_WASM
        LazyFS::Initialize();
        {
            std::string base_url = std::string("/assets/") + REQUIRED_ROOT_NX;
            if (!LazyFS::RegisterFile(REQUIRED_ROOT_NX, base_url))
            {
                return { Error::MISSING_FILE, REQUIRED_ROOT_NX };
            }
        }
        for (auto filename : NxFiles::filenames)
        {
            std::string url = std::string("/assets/") + filename;
            if (!LazyFS::RegisterFile(filename, url))
            {
                return { Error::MISSING_FILE, filename };
            }
        }
#else
        if (!std::ifstream{ REQUIRED_ROOT_NX }.good())
        {
            return { Error::MISSING_FILE, REQUIRED_ROOT_NX };
        }
        for (auto filename : NxFiles::filenames)
        {
            if (!std::ifstream{ filename }.good())
            {
                return { Error::MISSING_FILE, filename };
            }
        }
#endif

        try
        {
            nl::nx::load_all();
        }
        catch (const std::exception& ex)
        {
            static const std::string message = ex.what();

            return { Error::NLNX, message.c_str() };
        }

        constexpr const char* POSTCHAOS_BITMAP =
            "Login.img/WorldSelect/BtChannel/layer:bg";
        // Attempt to resolve it to log if this "version" identifier exists
        auto ui_node = nl::nx::ui.resolve(POSTCHAOS_BITMAP);
        
        if (!ui_node || ui_node.data_type() != nl::node::type::bitmap)
        {
            Console::get().print("[init] UI.nx is missing the post-Chaos 'Login.img/WorldSelect/BtChannel/layer:bg' node.");
            Console::get().print("[init] Client expected post-Chaos UI.nx version, but continuing anyway.");
        }
        return Error::NONE;
    }
}
