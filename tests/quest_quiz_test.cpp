#include "support/assets.h"
#include <doctest/doctest.h>
#include "client/Gameplay/QuestQuiz.h"
#include "client/IO/Components/NpcMenu.h"
#include "nlnx/nx.hpp"

TEST_CASE("Robin's quiz requires all three correct answers before completion")
{
    test_support::NxFile quest_file("Quest.nx", nl::nx::quest);
    const auto& robin = jrc::QuestData::get(1036);
    REQUIRE(robin.get_name() == "Robin the Walking Encyclopedia");
    REQUIRE_FALSE(robin.is_end_scripted());
    const auto questions = robin.get_quiz_questions(false);
    REQUIRE(questions.size() == 3);
    REQUIRE(robin.get_dialog(false).size() == 4);
    jrc::QuestQuiz quiz(questions);
    const int32_t correct[] = {1, 1, 3};
    const size_t choice_counts[] = {2, 5, 4};
    for (size_t page = 0; page < 3; ++page)
    {
        CAPTURE(page);
        CHECK_FALSE(quiz.complete());
        REQUIRE(quiz.is_question(page));
        const auto menu = jrc::NpcMenu::parse(robin.get_dialog(false)[page]);
        REQUIRE(menu.options.size() == choice_counts[page]);
        CHECK(menu.prompt.find("#L") == std::string::npos);
        CHECK(menu.prompt.find("\\r") == std::string::npos);
        CHECK(menu.prompt.find('\n') == std::string::npos);
        CHECK(questions.at(page).correct_selection == correct[page]);
        REQUIRE(quiz.answer(page, menu.options[correct[page]].id));
    }
    CHECK_FALSE(quiz.is_question(3));
    CHECK(quiz.complete());

    // Revisiting an earlier question and failing must invalidate the pass.
    CHECK_FALSE(quiz.answer(1, 4));
    CHECK_FALSE(quiz.complete());
    CHECK(quiz.feedback(1, 4).find("Level 8") != std::string::npos);
    CHECK(quiz.answer(2, 3));
    CHECK_FALSE(quiz.complete());
    CHECK(quiz.answer(0, 1));
    CHECK_FALSE(quiz.complete());
    CHECK(quiz.answer(1, 1));
    CHECK(quiz.complete());
}

TEST_CASE("Every wrong Robin answer retains its explanation and prevents completion")
{
    test_support::NxFile quest_file("Quest.nx", nl::nx::quest);
    const auto& robin = jrc::QuestData::get(1036);
    const auto questions = robin.get_quiz_questions(false);
    for (const auto& entry : questions)
    {
        const size_t page = entry.first;
        const auto& question = entry.second;
        for (const auto& option : jrc::NpcMenu::parse(robin.get_dialog(false)[page]).options)
        {
            if (option.id == question.correct_selection) continue;
            CAPTURE(page);
            CAPTURE(option.id);
            jrc::QuestQuiz quiz(questions);
            for (size_t previous = 0; previous < page; ++previous)
                REQUIRE(quiz.answer(previous, questions.at(previous).correct_selection));
            CHECK_FALSE(quiz.answer(page, option.id));
            CHECK_FALSE(quiz.complete());
            CHECK(quiz.feedback(page, option.id).find("Incorrect!") == 0);
            for (size_t following = page; following < 3; ++following)
                REQUIRE(quiz.answer(following, questions.at(following).correct_selection));
            CHECK(quiz.complete() == (page == 0));
        }
    }
}

TEST_CASE("Quiz metadata supports other quests and leaves ordinary dialogue alone")
{
    test_support::NxFile quest_file("Quest.nx", nl::nx::quest);
    const auto& rain = jrc::QuestData::get(1009);
    jrc::QuestQuiz quiz(rain.get_quiz_questions(false));
    REQUIRE(quiz.is_question(0));
    CHECK_FALSE(quiz.answer(0, 1));
    CHECK(quiz.feedback(0, 1).find("Skill Window") != std::string::npos);
    CHECK_FALSE(quiz.answer(0, 2));
    CHECK(quiz.feedback(0, 2).find("ability stats") != std::string::npos);
    CHECK_FALSE(quiz.complete());
    CHECK(quiz.answer(0, 0));
    CHECK(quiz.complete());
    CHECK(jrc::QuestData::get(1036).get_quiz_questions(true).empty());
    CHECK(jrc::QuestData::get(1040).get_quiz_questions(true).empty());
    CHECK(jrc::QuestQuiz().complete());
    CHECK_FALSE(jrc::QuestQuiz().answer(0, 0));
}
