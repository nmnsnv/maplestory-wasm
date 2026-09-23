#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Character/CharStats.h"
#include "client/Data/EquipData.h"
#include "nlnx/file.hpp"
#include "nlnx/nx.hpp"


namespace
{
    using namespace jrc;

    CharStats character(uint16_t job, uint16_t level)
    {
        StatsEntry entry{};
        entry.stats.clear();
        entry.stats[Maplestat::LEVEL] = level;
        entry.stats[Maplestat::JOB] = job;
        for (auto stat : {Maplestat::STR, Maplestat::DEX, Maplestat::INT, Maplestat::LUK})
            entry.stats[stat] = 999;
        return CharStats(entry);
    }
}

namespace jrc
{
    // Metadata tests do not need a graphics context; all requirement data
    // and character calculations still use the production implementations.
    Texture::Texture() {}
    Texture::Texture(nl::node) {}
    Texture::~Texture() {}
}

TEST_CASE("Equipment eligibility and tooltip requirements agree")
{
    test_support::NxFile character_file("Character.nx", nl::nx::character);
    test_support::NxFile string_file("String.nx", nl::nx::string);

    int32_t skirt_id = 0;
    for (auto item : nl::nx::string["Eqp.img"]["Eqp"]["Pants"])
    {
        if (item["name"].get_string() == "Green Avelin Skirt")
            skirt_id = std::stoi(item.name());
    }
    REQUIRE_MESSAGE((skirt_id != 0), "Regression fixture must be the skirt from the screenshot");
    const EquipData& skirt = EquipData::get(skirt_id);
    REQUIRE_MESSAGE((skirt.is_valid()), "Skirt must load from the real equipment data");
    REQUIRE_MESSAGE((skirt.get_reqstat(Maplestat::JOB) == 4), "Skirt must require a bowman");
    REQUIRE_MESSAGE((skirt.get_reqstat(Maplestat::LEVEL) == 10), "Skirt must require level 10");

    for (uint16_t job : {0, 100, 200, 400, 500, 1000, 1100, 1200, 1400, 1500, 2000, 2112})
    {
        auto stats = character(job, 200);
        REQUIRE_MESSAGE((!skirt.can_equip(stats)), "High level and stats cannot bypass the skirt's class restriction");
        REQUIRE_MESSAGE((!skirt.meets_requirement(Maplestat::JOB, stats)), "Tooltip and equip action must both reject wrong class");
    }
    for (uint16_t job : {300, 312, 322, 1300, 1311})
    {
        auto stats = character(job, 10);
        REQUIRE_MESSAGE((skirt.can_equip(stats)), "Eligible bowmen must still be able to equip the skirt");
        REQUIRE_MESSAGE((skirt.meets_requirement(Maplestat::JOB, stats)), "Tooltip must accept eligible bowmen");
        stats.set_stat(Maplestat::LEVEL, 9);
        REQUIRE_MESSAGE((!skirt.can_equip(stats)), "Correct class cannot bypass the level requirement");
    }

    // Use real weapons to exercise each stat boundary and equipped bonuses.
    const std::pair<Maplestat::Id, Equipstat::Id> stat_pairs[] = {
        {Maplestat::STR, Equipstat::STR}, {Maplestat::DEX, Equipstat::DEX},
        {Maplestat::INT, Equipstat::INT}, {Maplestat::LUK, Equipstat::LUK}
    };
    for (const auto& [base_stat, total_stat] : stat_pairs)
    {
        bool found = false;
        for (auto item : nl::nx::character["Weapon"])
        {
            const std::string name = item.name();
            if (name.size() != 12 || name.substr(8) != ".img")
                continue;
            const auto& data = EquipData::get(std::stoi(name.substr(0, 8)));
            const int16_t required = data.get_reqstat(base_stat);
            if (!data.is_valid() || required <= 0 || required > 999)
                continue;

            for (uint16_t job : {0, 100, 200, 300, 400, 500})
            {
                auto stats = character(job, 255);
                if (!data.can_equip(stats))
                    continue;
                stats.set_stat(base_stat, required - 1);
                stats.set_total(total_stat, required - 1);
                REQUIRE_MESSAGE((!data.can_equip(stats)), "An unmet stat requirement must block equipping");
                REQUIRE_MESSAGE((!data.meets_requirement(base_stat, stats)), "Tooltip must mark the unmet stat");
                stats.set_total(total_stat, required);
                REQUIRE_MESSAGE((data.can_equip(stats)), "Equipped bonuses can satisfy the stat requirement exactly");
                REQUIRE_MESSAGE((data.meets_requirement(base_stat, stats)), "Tooltip must also count equipped bonuses");
                found = true;
                break;
            }
            if (found)
                break;
        }
        REQUIRE_MESSAGE((found), "A real equipment fixture must cover each stat requirement");
    }
    REQUIRE_MESSAGE((!EquipData::get(0).can_equip(character(300, 200))), "Invalid item data must not allow equipping");
}
