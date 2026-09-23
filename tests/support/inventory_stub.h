#pragma once

#include <cstdint>
#include <map>

namespace test_support
{
    inline std::map<int32_t, int32_t> held_items;

    struct InventoryFixture
    {
        InventoryFixture() { held_items.clear(); }
        ~InventoryFixture() { held_items.clear(); }
        InventoryFixture(const InventoryFixture&) = delete;
        InventoryFixture& operator=(const InventoryFixture&) = delete;
    };
}
