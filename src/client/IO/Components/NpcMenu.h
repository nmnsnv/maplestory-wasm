#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace jrc
{
    struct NpcMenu
    {
        struct Option
        {
            int32_t id;
            std::string text;
        };
        std::string prompt;
        std::vector<Option> options;

        static NpcMenu parse(const std::string& source);
    };
}
