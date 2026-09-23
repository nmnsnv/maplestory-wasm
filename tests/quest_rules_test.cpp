#include "support/inventory_stub.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Character/Inventory/Inventory.h"
#include "client/Character/QuestLog.h"
#include "client/Data/QuestData.h"
#include "nlnx/file.hpp"
#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    // Inventory is the boundary of these quest-rule tests; isolate it from
    // graphics and equipment metadata while exercising the real NX loader.
    using test_support::held_items;

    constexpr int64_t NOW = 134200000000000000LL;
    constexpr int64_t MINUTE = 600000000LL;
    using Eligibility = jrc::Questlog::Eligibility;
}

TEST_CASE("Quest eligibility respects requirements and repeat cooldowns")
{
    test_support::InventoryFixture inventory_state;

    test_support::NxFile quest_file("Quest.nx", nl::nx::quest);
    const jrc::Inventory inventory;
    auto eligible = [&](const jrc::Questlog& log, int16_t qid, bool start,
                        uint16_t level = 70, uint16_t job = 100, int32_t map = 0) {
        return log.get_eligibility(qid, start, level, job, inventory, map);
    };

    jrc::Questlog apple;
    apple.add_started(1021, "");
    REQUIRE_MESSAGE((jrc::QuestData::get(1021).get_item_requirements().at(0).count == 0),
        "Roger's Apple must default a missing requirement count to zero");
    held_items[2010007] = 1;
    REQUIRE_MESSAGE((eligible(apple, 1021, false) == Eligibility::UNAVAILABLE), "Apple must be consumed first");
    held_items.clear();
    REQUIRE_MESSAGE((eligible(apple, 1021, false) == Eligibility::AVAILABLE), "Consumed apple must allow completion");

    jrc::Questlog zero_item;
    zero_item.add_started(8220, "");
    held_items[4032018] = 1;
    REQUIRE_MESSAGE((eligible(zero_item, 8220, false) == Eligibility::UNAVAILABLE), "Explicit zero must reject held item");
    held_items.clear();
    REQUIRE_MESSAGE((eligible(zero_item, 8220, false) == Eligibility::AVAILABLE), "Explicit zero must accept absence");
    REQUIRE_MESSAGE((eligible(zero_item, 8850, true, 35, 0) == Eligibility::AVAILABLE), "Start absence requirement should pass");
    held_items[4031563] = 1;
    REQUIRE_MESSAGE((eligible(zero_item, 8850, true, 35, 0) == Eligibility::UNAVAILABLE), "Start absence requirement should fail when held");
    held_items.clear();
    zero_item.add_started(2020, "");
    REQUIRE_MESSAGE((eligible(zero_item, 2021, true) == Eligibility::AVAILABLE), "Missing start item count must mean absence");
    held_items[4031014] = 1;
    REQUIRE_MESSAGE((eligible(zero_item, 2021, true) == Eligibility::UNAVAILABLE), "Missing start item count must reject possession");
    held_items.clear();

    jrc::Questlog chief;
    chief.add_started(1040, "");
    REQUIRE_MESSAGE((eligible(chief, 1040, false) == Eligibility::UNAVAILABLE), "Chief must require all Mai quests");
    for (int16_t qid : {1041, 1042, 1043})
        chief.add_completed(qid, NOW);
    REQUIRE_MESSAGE((eligible(chief, 1040, false) == Eligibility::UNAVAILABLE), "Partial Mai training must not complete Chief");
    chief.add_completed(1044, NOW);
    REQUIRE_MESSAGE((eligible(chief, 1040, false) == Eligibility::AVAILABLE), "All Mai training should complete Chief");

    jrc::Questlog exclusive;
    REQUIRE_MESSAGE((eligible(exclusive, 2120, true) == Eligibility::AVAILABLE), "State zero should allow unstarted prerequisite");
    exclusive.add_started(3242, "");
    REQUIRE_MESSAGE((eligible(exclusive, 2120, true) == Eligibility::UNAVAILABLE), "State zero must reject active prerequisite");
    exclusive.complete(3242, NOW);
    REQUIRE_MESSAGE((eligible(exclusive, 2120, true) == Eligibility::UNAVAILABLE), "State zero must reject completed prerequisite");

    jrc::Questlog repeats;
    repeats.set_server_time(NOW);
    repeats.add_completed(2017, NOW);
    REQUIRE_MESSAGE((eligible(repeats, 2017, true) == Eligibility::AVAILABLE), "Arwen must allow immediate repeats");
    repeats.add_completed(1040, NOW);
    REQUIRE_MESSAGE((eligible(repeats, 1040, true, 5, 0) == Eligibility::UNAVAILABLE), "Non-repeatable quest must stay completed");
    repeats.add_completed(8850, NOW - 1439 * MINUTE);
    REQUIRE_MESSAGE((eligible(repeats, 8850, true, 35, 0) == Eligibility::UNAVAILABLE), "Daily cooldown must block early repeat");
    repeats.add_completed(8850, NOW - 1440 * MINUTE);
    REQUIRE_MESSAGE((eligible(repeats, 8850, true, 35, 0) == Eligibility::AVAILABLE), "Daily cooldown must end on server time");
    repeats.add_completed(8850, NOW + MINUTE);
    REQUIRE_MESSAGE((eligible(repeats, 8850, true, 35, 0) == Eligibility::UNAVAILABLE), "Future timestamp must not bypass cooldown");
    repeats.update_progress(2017, "");
    REQUIRE_MESSAGE((repeats.is_active(2017) && !repeats.is_completed(2017)), "Restart must replace completed state");
    REQUIRE_MESSAGE((eligible(repeats, 2017, true) == Eligibility::UNAVAILABLE), "Active repeat must not be offered again");
    repeats.remove_active(2017);
    REQUIRE_MESSAGE((eligible(repeats, 2017, true) == Eligibility::AVAILABLE), "Forfeited repeat should be startable");

    jrc::Questlog dates;
    dates.set_server_time(NOW);
    REQUIRE_MESSAGE((eligible(dates, 28107, true, 10, 0) == Eligibility::UNAVAILABLE), "Expired Cody event must be hidden");
    // November 2007, before Cosmic's normalized December 27 deadline.
    dates.set_server_time(116444736010800000LL + 1193875200000LL * 10000);
    REQUIRE_MESSAGE((eligible(dates, 28107, true, 10, 0) == Eligibility::AVAILABLE), "Event should be available before deadline");
    dates.set_server_time(116444736010800000LL + 1196467200000LL * 10000);
    REQUIRE_MESSAGE((eligible(dates, 28107, true, 10, 0) == Eligibility::AVAILABLE), "Deadline must match Cosmic's Calendar month");
    dates.set_server_time(116444736010800000LL + 1198800000000LL * 10000);
    REQUIRE_MESSAGE((eligible(dates, 28107, true, 10, 0) == Eligibility::UNAVAILABLE), "Event must expire after normalized deadline");

    jrc::Questlog progress;
    REQUIRE_MESSAGE((eligible(progress, 1047, true) == Eligibility::UNAVAILABLE), "Map-bound quest must reject another map");
    REQUIRE_MESSAGE((eligible(progress, 1047, true, 70, 100, 999999999) == Eligibility::AVAILABLE), "Map-bound quest must allow its map");
    progress.add_started(20706, "");
    REQUIRE_MESSAGE((eligible(progress, 20706, false) == Eligibility::UNAVAILABLE), "Info-number completion must require progress");
    progress.update_progress(20731, "1");
    REQUIRE_MESSAGE((eligible(progress, 20706, false) == Eligibility::AVAILABLE), "Info-number completion must use linked record");
    progress.add_started(1041, "004");
    held_items[4000003] = 3;
    REQUIRE_MESSAGE((eligible(progress, 1041, false) == Eligibility::UNAVAILABLE), "Mob requirement must reject partial kills");
    progress.update_progress(1041, "005");
    REQUIRE_MESSAGE((eligible(progress, 1041, false) == Eligibility::AVAILABLE), "Mob and item requirements should combine");
    held_items.clear();
    REQUIRE_MESSAGE((eligible(progress, 1041, false) == Eligibility::UNAVAILABLE), "Missing gathered items must block hand-in");
    progress.add_started(2103, "");
    REQUIRE_MESSAGE((eligible(progress, 2103, false) == Eligibility::SERVER_CHECK),
        "Pet conditions must remain reachable without claiming completion");

    const std::vector<int32_t> warrior_rewards = {2043002, 2043102, 2043202, 2044002, 2044102, 2044202, 2044302, 2044402};
    for (uint16_t job : {100, 112, 1100, 1112, 2100, 2112})
    {
        std::vector<int32_t> choices;
        for (const auto& reward : jrc::QuestData::get(2119).get_item_rewards())
            if (reward.prop == -1 && jrc::QuestData::is_reward_eligible(reward, false, job))
                choices.push_back(reward.id);
        REQUIRE_MESSAGE((choices == warrior_rewards), ("Numeric reward order and job flags must match Cosmic for job " + std::to_string(job)));
    }
    jrc::QuestData::ItemReward reward{1, 1, -1, 1, 0};
    REQUIRE_MESSAGE((!jrc::QuestData::is_reward_eligible(reward, false, 100)), "Gender restriction must filter reward");
    REQUIRE_MESSAGE((jrc::QuestData::is_reward_eligible(reward, true, 100)), "Eligible gender must retain reward");
    reward.gender = 2;
    reward.job = 0x400000;
    REQUIRE_MESSAGE((jrc::QuestData::is_reward_eligible(reward, false, 2210)), "Evan family flag must be supported");
    REQUIRE_MESSAGE((!jrc::QuestData::is_reward_eligible(reward, false, 2100)), "Evan flag must not match Aran");

    const auto instructions = jrc::QuestData::get(1040).get_dialog_branch(true, "yes");
    REQUIRE_MESSAGE((!instructions.empty() && instructions.front().find("Mai") != std::string::npos),
        "Chief's accepted branch must retain directions to Mai");
    REQUIRE_MESSAGE((jrc::QuestData::get(1040).get_dialog_branch(false, "yes").size() == 2),
        "Completion follow-up must retain both instruction pages");
    REQUIRE_MESSAGE((!jrc::QuestData::get(1040).get_dialog_branch(true, "no").empty()), "Decline branch must be available");
    REQUIRE_MESSAGE((jrc::QuestData::get(1040).get_dialog_branch(true, "missing").empty()), "Missing branch must be harmless");
    const auto& npc_quests = jrc::QuestData::quests_by_npc(1032100);
    REQUIRE_MESSAGE((std::is_sorted(npc_quests.begin(), npc_quests.end())), "NPC quest ordering must be stable and numeric");
    REQUIRE_MESSAGE((npc_quests.size() > 1), "NPC menu fixture must include multiple quest choices");
}
