#include "support/packet_fixture.h"
#include "client/Net/Packets/CashShopPackets.h"
#include "client/Net/Packets/InventoryPackets.h"
#include "client/Net/Handlers/Helpers/ItemParser.h"
#include "client/Character/Inventory/EquipQuality.h"
#include "client/Graphics/Texture.h"

// Quality colors are unrelated to inventory identity and require rendering assets.
namespace jrc { Texture::~Texture() {} }
namespace jrc::EquipQuality
{
    Id check_quality(int32_t, bool, const EnumMap<Equipstat::Id, uint16_t>&) { return WHITE; }
}
namespace
{
    using namespace jrc;
    using test_support::expect_packet;
    // Fixture writer for Cosmic's itemInfo records, including unused protocol fields.
    struct Fixture : test_support::PacketFixture
    {
        Fixture(uint8_t kind, int32_t item, int64_t id) : PacketFixture()
        {
            write_byte(kind); write_int(item); write_byte(id != 0);
            if (id) write_long(id);
            write_long(133500000000000000LL);
        }
    };
}
TEST_CASE("Cash shop requests match the v83 wire layout")
{
    using namespace jrc;
    // Fixed fixtures follow Cosmic's readers; there is no client-side quantity in a buy request.
    expect_packet(CashPurchasePacket(0x01020304, 4, 3), {0xE5,0,3,0,4,0,0,0,4,3,2,1});
    expect_packet(CashPurchasePacket(0x01020304, 2, 0x1E), {0xE5,0,0x1E,0,2,0,0,0,4,3,2,1});
    expect_packet(CashPurchasePacket(0x01020304, 1, 0x20), {0xE5,0,0x20,4,3,2,1});
    expect_packet(CashMovePacket(0x01020304, 1, false), {0xE5,0,0x0D,4,3,2,1,0,0,0,0});
    expect_packet(CashMovePacket(0x01020304, 5, true), {0xE5,0,0x0E,4,3,2,1,0,0,0,0,5});
    expect_packet(CashCouponPacket("ABC"), {0xE6,0,0,0,3,0,'A','B','C'});
    expect_packet(CashGiftPacket(0x01020304, 0x05060708, "Bob", "Hi"),
           {0xE5,0,4,8,7,6,5,4,3,2,1,3,0,'B','o','b',2,0,'H','i'});
    expect_packet(CashExpandPacket(1, 1), {0xE5,0,6,0,1,0,0,0,0,1});
    expect_packet(CashExpandPacket(4, 0), {0xE5,0,7,0,4,0,0,0,0});
    const CashWishlistPacket wishes({1,2,3,4,5,6,7,8,9,10});
    InPacket wishlist(wishes.data().data(), wishes.data().size());
    REQUIRE((wishlist.read_short() == 0xE5 && wishlist.read_byte() == 5));
    for (int32_t i = 1; i <= 10; ++i) REQUIRE((wishlist.read_int() == i));
    REQUIRE((!wishlist.available()));
}

TEST_CASE("Cash equipment requests target the cash equipment slot")
{
    using namespace jrc;
    const EquipItemPacket equip(2, Equipslot::CAP, true);
    InPacket equipment(equip.data().data(), equip.data().size());
    equipment.skip(7);
    REQUIRE((equipment.read_short() == 2 && equipment.read_short() == -101));
}

TEST_CASE("Equipping and unequipping cash items preserves identity and expiration")
{
    using namespace jrc;
    Inventory inventory;
    inventory.set_slotmax(InventoryType::EQUIP, 2);
    Fixture hat(1, 1000000, 123456);
    hat.write_byte(0); hat.write_byte(0); hat.skip(30); hat.write_string("");
    hat.write_short(0); hat.skip(22);
    auto hat_input = hat.input();
    ItemParser::parse_item(hat_input, InventoryType::EQUIP, 1, inventory);
    REQUIRE((!hat_input.available()));
    REQUIRE((inventory.is_cash(InventoryType::EQUIP, 1)));
    REQUIRE((inventory.get_cash_id(InventoryType::EQUIP, 1) == 123456));
    REQUIRE((inventory.get_expiration(InventoryType::EQUIP, 1) == 133500000000000000LL));
    REQUIRE((inventory.find_free_slot(InventoryType::EQUIP) == 2));
    inventory.modify(InventoryType::EQUIP, 1, Inventory::SWAP, -101, Inventory::MOVE_EQUIP);
    REQUIRE((inventory.get_cash_id(InventoryType::EQUIPPED, 101) == 123456));
    REQUIRE((!inventory.get_cash_id(InventoryType::EQUIP, 1)));
    inventory.modify(InventoryType::EQUIP, -101, Inventory::SWAP, 2, Inventory::MOVE_UNEQUIP);
    REQUIRE((inventory.get_cash_id(InventoryType::EQUIP, 2) == 123456));
    REQUIRE((!inventory.get_cash_id(InventoryType::EQUIPPED, 101)));
    auto another_hat = hat.input();
    ItemParser::parse_item(another_hat, InventoryType::EQUIP, 1, inventory);
    REQUIRE((inventory.find_free_slot(InventoryType::EQUIP) == 0));
    inventory.modify(InventoryType::EQUIP, 1, Inventory::REMOVE, 0, Inventory::MOVE_NONE);
    REQUIRE((inventory.find_free_slot(InventoryType::EQUIP) == 1));

    InPacket missing(hat.data().data() + 2, hat.data().size() - 3);
    CHECK_THROWS_AS(ItemParser::parse_item(missing, InventoryType::EQUIP, 1, inventory), PacketError);
}

TEST_CASE("Pet and consumable records preserve cash IDs and quantities")
{
    using namespace jrc;
    Inventory inventory;
    Fixture pet(3, 5000000, 654321);
    pet.skip(13); pet.write_byte(1); pet.write_short(10); pet.write_byte(100); pet.skip(18);
    auto pet_input = pet.input();
    ItemParser::parse_item(pet_input, InventoryType::CASH, 1, inventory);
    REQUIRE((!pet_input.available()));
    REQUIRE((inventory.get_cash_id(InventoryType::CASH, 1) == 654321));
    Fixture consumable(2, 2000000, 98765);
    consumable.write_short(30); consumable.write_string(""); consumable.write_short(0);
    auto consume_input = consumable.input();
    ItemParser::parse_item(consume_input, InventoryType::USE, 1, inventory);
    REQUIRE((!consume_input.available()));
    REQUIRE((inventory.get_item_count(InventoryType::USE, 1) == 30));
    REQUIRE((inventory.get_cash_id(InventoryType::USE, 1) == 98765));
}
