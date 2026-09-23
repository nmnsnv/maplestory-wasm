#include "inventory_stub.h"
#include "client/Character/Inventory/Inventory.h"

namespace jrc
{
    // Quest scenarios control inventory contents without bringing in equipment
    // rendering. Quest parsing and eligibility still use production code.
    Inventory::Inventory() : bulletslot(0), meso(0), running_uid(0) {}
    int32_t Inventory::count_items(int32_t id) const
    {
        const auto item = test_support::held_items.find(id);
        return item == test_support::held_items.end() ? 0 : item->second;
    }
    int64_t Inventory::get_meso() const { return meso; }
}
