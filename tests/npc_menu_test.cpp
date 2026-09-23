#include <doctest/doctest.h>
#include "client/IO/Components/NpcMenu.h"

TEST_CASE("NPC menu options preserve IDs and tolerate omitted closing tags")
{
    const auto menu = jrc::NpcMenu::parse("Pick one:\r\n#b#L7#First\r\n#L42#Second#l\r\n#L99#Last");
    REQUIRE(menu.options.size() == 3);
    CHECK(menu.prompt == "Pick one:");
    CHECK(menu.options[0].id == 7);
    CHECK(menu.options[0].text == "First");
    CHECK(menu.options[1].id == 42);
    CHECK(menu.options[1].text == "Second");
    CHECK(menu.options[2].id == 99);
    CHECK(menu.options[2].text == "Last");
}

TEST_CASE("Local quest escapes and network newlines produce the same menu")
{
    const auto local = jrc::NpcMenu::parse("Question?\\r\\n#b#L0# Yes #l\\r\\n#b#L1# No #k#l\\r\\n");
    const auto network = jrc::NpcMenu::parse("Question?\r\n#b#L0# Yes #l\r\n#b#L1# No #k#l\r\n");
    REQUIRE(local.options.size() == 2);
    REQUIRE(network.options.size() == 2);
    CHECK(local.prompt == "Question?");
    CHECK(local.prompt == network.prompt);
    for (size_t i = 0; i < local.options.size(); ++i)
    {
        CHECK(local.options[i].id == network.options[i].id);
        CHECK(local.options[i].text == network.options[i].text);
        CHECK(local.options[i].text.find('\n') == std::string::npos);
    }
    const auto multiline = jrc::NpcMenu::parse("#L0#Line one\\nLine two#l");
    REQUIRE(multiline.options.size() == 1);
    CHECK(multiline.options.front().text == "Line one\nLine two");
}

TEST_CASE("Malformed NPC choice IDs do not consume later valid answers")
{
    const auto menu = jrc::NpcMenu::parse("Bad #L# #L-1# #L999999999999999999#\n#L2147483647#Valid#l");
    REQUIRE(menu.options.size() == 1);
    CHECK(menu.options.front().id == 2147483647);
    CHECK(menu.options.front().text == "Valid");
    CHECK(menu.prompt.find("#L999999999999999999#") != std::string::npos);
    CHECK(jrc::NpcMenu::parse("No choices here.").prompt == "No choices here.");
    CHECK(jrc::NpcMenu::parse("").options.empty());
}
