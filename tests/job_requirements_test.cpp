#include <doctest/doctest.h>
#include "client/Character/Job.h"
#include <utility>

using jrc::Job;

TEST_CASE("All beginner branches can wear beginner and common equipment")
{
    for (uint16_t job : {0, 1000, 2000, 2001})
    {
        REQUIRE_MESSAGE((Job(job).can_equip(0)), "Beginners can wear common equipment");
        REQUIRE_MESSAGE((Job(job).can_equip(-1)), "Beginner-only equipment supports every beginner branch");
        REQUIRE_MESSAGE((!Job(job).can_equip(31)), "Beginners cannot wear advanced-class equipment");
    }
}

TEST_CASE("Advanced jobs retain their equipment class across advancements")
{
    const std::pair<uint16_t, int16_t> classes[] = {
        {100, 1}, {112, 1}, {132, 1}, {1100, 1}, {1111, 1}, {2100, 1}, {2112, 1},
        {200, 2}, {232, 2}, {1200, 2}, {1211, 2}, {2200, 2}, {2218, 2},
        {300, 4}, {312, 4}, {322, 4}, {1300, 4}, {1311, 4},
        {400, 8}, {422, 8}, {1400, 8}, {1411, 8},
        {500, 16}, {522, 16}, {1500, 16}, {1511, 16}
    };
    for (const auto& [job, mask] : classes)
    {
        REQUIRE_MESSAGE((Job(job).can_equip(mask)), "Job advancement must preserve equipment class");
        REQUIRE_MESSAGE((Job(job).can_equip(0)), "Common equipment supports advanced jobs");
        REQUIRE_MESSAGE((!Job(job).can_equip(-1)), "Advanced jobs cannot wear beginner-only equipment");
        REQUIRE_MESSAGE((!Job(job).can_equip(31 ^ mask)), "Every other class must be rejected");
    }
}

TEST_CASE("Combined and invalid equipment masks respect class restrictions")
{
    REQUIRE_MESSAGE((Job(112).can_equip(3) && Job(232).can_equip(3)), "Combined class masks allow either class");
    REQUIRE_MESSAGE((!Job(312).can_equip(3)), "Combined class masks reject unrelated classes");
    REQUIRE_MESSAGE((Job(422).can_equip(24) && Job(522).can_equip(24)), "Nonconsecutive class masks work");
    REQUIRE_MESSAGE((!Job(900).can_equip(31) && !Job(9999).can_equip(31)), "Unknown classes cannot bypass restrictions");
    REQUIRE_MESSAGE((!Job(300).can_equip(-2)), "Invalid negative class masks are rejected");
}
