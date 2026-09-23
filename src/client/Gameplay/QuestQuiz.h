#pragma once

#include "../Data/QuestData.h"

#include <set>
#include <utility>

namespace jrc
{
    // Keep quiz progress separate from page navigation: viewing the final
    // page or choosing an item reward must never bypass unanswered questions.
    class QuestQuiz
    {
    public:
        QuestQuiz() = default;
        explicit QuestQuiz(std::map<size_t, QuestData::QuizQuestion> questions)
            : questions(std::move(questions)) {}

        bool is_question(size_t page) const { return questions.count(page) != 0; }
        bool complete() const { return answered.size() == questions.size(); }

        bool answer(size_t page, int32_t selection)
        {
            const auto question = questions.find(page);
            if (question == questions.end()) return false;
            if (selection != question->second.correct_selection)
            {
                // A failed attempt starts over, including previously passed
                // questions, so going back cannot preserve a partial pass.
                answered.clear();
                return false;
            }
            answered.insert(page);
            return true;
        }

        std::string feedback(size_t page, int32_t selection) const
        {
            const auto question = questions.find(page);
            if (question != questions.end())
            {
                const auto response = question->second.incorrect_responses.find(selection);
                if (response != question->second.incorrect_responses.end() && !response->second.empty())
                    return response->second;
            }
            return "That answer is incorrect. Please try again.";
        }

    private:
        std::map<size_t, QuestData::QuizQuestion> questions;
        std::set<size_t> answered;
    };
}
