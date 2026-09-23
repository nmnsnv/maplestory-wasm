#include "support/inventory_stub.h"
#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Gameplay/QuestConversation.h"
#include "client/Character/Inventory/Inventory.h"
#include "client/Data/QuestData.h"
#include "nlnx/file.hpp"
#include "nlnx/node.hpp"
#include "nlnx/nx.hpp"

#include <algorithm>
#include <stdexcept>

namespace
{
    using test_support::held_items;
    constexpr int64_t NOW = 134200000000000000LL;
    using Conversation = jrc::QuestConversation;
    using Action = Conversation::Action;
    using Route = Conversation::Route;
    using Eligibility = jrc::Questlog::Eligibility;

    const Conversation::Option& find(const Conversation& conversation, int16_t qid)
    {
        const auto found = std::find_if(conversation.options.begin(), conversation.options.end(),
            [qid](const Conversation::Option& option) { return option.qid == qid; });
        if (found == conversation.options.end())
            throw std::runtime_error("Missing conversation for quest " + std::to_string(qid));
        return *found;
    }
}

TEST_CASE("Quest conversations follow progress and available actions")
{
    test_support::InventoryFixture inventory_state;

    test_support::NxFile quest_file("Quest.nx", nl::nx::quest);
    const jrc::Inventory inventory;
    auto build = [&](const jrc::Questlog& quests, int32_t npcid, bool services = false,
                     uint16_t level = 5, uint16_t job = 0, int32_t map_id = 0) {
        return Conversation::build(npcid, services, quests, level, job, inventory, map_id);
    };

    jrc::Questlog roger;
    REQUIRE_MESSAGE((build(roger, 9999999).route() == Route::NPC), "No quests must leave regular NPC talk in charge");
    REQUIRE_MESSAGE((build(roger, 9999999, true).route() == Route::NPC), "Services alone must not create a quest chooser");
    auto conversation = build(roger, 2000);
    REQUIRE_MESSAGE((conversation.route() == Route::QUEST), "Roger's lone quest must open directly");
    REQUIRE_MESSAGE((conversation.options.front().qid == 1021 && conversation.options.front().action == Action::START),
        "Direct conversation must identify the available quest");
    REQUIRE_MESSAGE((jrc::QuestData::get(1021).is_start_scripted()), "Direct opening must also cover server-scripted quests");
    REQUIRE_MESSAGE((build(roger, 2000, true).route() == Route::QUEST), "A service entry must not defeat the lone-quest shortcut");
    REQUIRE_MESSAGE((!roger.is_active(1021)), "Opening a conversation must not accept a quest locally");

    roger.add_started(1021, "");
    held_items[2010007] = 1;
    conversation = build(roger, 2000);
    REQUIRE_MESSAGE((conversation.route() == Route::QUEST), "Lone in-progress quests must also open directly");
    REQUIRE_MESSAGE((find(conversation, 1021).action == Action::PROGRESS), "An uneaten apple must open progress, not completion");
    held_items.clear();
    conversation = build(roger, 2000);
    REQUIRE_MESSAGE((conversation.route() == Route::QUEST && find(conversation, 1021).action == Action::COMPLETE),
        "Eating the apple must switch the same direct conversation to completion");
    held_items[2010007] = 1;
    REQUIRE_MESSAGE((find(build(roger, 2000), 1021).action == Action::PROGRESS),
        "Lost completion eligibility must be rechecked when choosing a quest");
    held_items.clear();
    roger.complete(1021, NOW);
    REQUIRE_MESSAGE((build(roger, 2000).route() == Route::NPC), "Completed non-repeatable quests must leave the conversation");

    jrc::Questlog mai;
    mai.add_started(1040, "");
    conversation = build(mai, 12100);
    REQUIRE_MESSAGE((conversation.route() == Route::QUEST && find(conversation, 1041).action == Action::START),
        "Mai's unlocked training must open directly");
    REQUIRE_MESSAGE((!jrc::QuestData::get(1041).is_start_scripted()), "Direct opening must also cover local quest dialogue");
    mai.add_started(1041, "004");
    held_items[4000003] = 3;
    REQUIRE_MESSAGE((find(build(mai, 12100), 1041).action == Action::PROGRESS), "Partial kills must keep the progress conversation");
    mai.update_progress(1041, "005");
    REQUIRE_MESSAGE((find(build(mai, 12100), 1041).action == Action::COMPLETE), "Live kills and inventory must enable hand-in");
    REQUIRE_MESSAGE((build(mai, 12100).options.size() == 1), "Active quests must not also appear as starts");
    mai.complete(1041, NOW);
    conversation = build(mai, 12100);
    REQUIRE_MESSAGE((conversation.route() == Route::QUEST && conversation.options.front().qid == 1042),
        "Completing a quest must expose the next quest directly");
    held_items.clear();

    jrc::Questlog arwen;
    arwen.set_server_time(NOW);
    arwen.add_started(8048, "000");
    arwen.add_started(2017, "");
    held_items[4001000] = 1;
    conversation = build(arwen, 1032100, true, 50, 100);
    REQUIRE_MESSAGE((conversation.route() == Route::MENU), "Several quests must open a chooser");
    REQUIRE_MESSAGE((conversation.options.front().action == Action::PROGRESS), "Ongoing quests must precede new quests");
    REQUIRE_MESSAGE((std::is_sorted(conversation.options.begin(), conversation.options.end(),
        [](const auto& left, const auto& right) { return left.action < right.action; })),
        "Chooser order must be progress, available, completion, then NPC services");
    REQUIRE_MESSAGE((find(conversation, 2017).action == Action::COMPLETE), "Ready quests must be offered alongside other quests");
    REQUIRE_MESSAGE((find(conversation, 2222).action == Action::START), "New quests must remain selectable alongside hand-ins");
    REQUIRE_MESSAGE((conversation.options.back().action == Action::TALK), "Scripted NPC services must remain reachable in the chooser");
    const auto without_services = build(arwen, 1032100, false, 50, 100);
    REQUIRE_MESSAGE((without_services.options.size() + 1 == conversation.options.size()),
        "NPCs without services must not get a fabricated Talk option");
    const auto before_start = build(arwen, 1032001, false, 50, 100);
    REQUIRE_MESSAGE((std::none_of(before_start.options.begin(), before_start.options.end(),
        [](const auto& option) { return option.qid == 2222; })),
        "An end NPC must not offer an unstarted quest belonging to another NPC");
    arwen.add_started(2222, "");
    REQUIRE_MESSAGE((find(build(arwen, 1032100, true, 50, 100), 2222).action == Action::PROGRESS),
        "Start NPCs must refer active quests to their designated end NPC");
    REQUIRE_MESSAGE((find(build(arwen, 1032001, false, 50, 100), 2222).action == Action::COMPLETE),
        "End NPCs must collect eligible active quests started elsewhere");

    held_items.clear();
    REQUIRE_MESSAGE((find(build(arwen, 1032100, true, 50, 100), 2017).action == Action::PROGRESS),
        "An open chooser must not retain a hand-in after its required item is removed");
    arwen.complete(2017, NOW);
    REQUIRE_MESSAGE((find(build(arwen, 1032100, true, 50, 100), 2017).action == Action::START),
        "Repeatable quests must return as starts after server-confirmed completion");

    jrc::Questlog pet;
    pet.add_started(2103, "");
    const auto at_pet_npc = build(pet, 1040002, true, 10, 0);
    REQUIRE_MESSAGE((find(at_pet_npc, 2103).action == Action::COMPLETE &&
        find(at_pet_npc, 2103).eligibility == Eligibility::SERVER_CHECK),
        "Server-only conditions must remain reachable without claiming confirmed eligibility");
    REQUIRE_MESSAGE((find(build(pet, 1052103, true, 10, 0), 2103).action == Action::PROGRESS),
        "An active quest must only offer hand-in at its end NPC");
}
