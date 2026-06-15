// Characterization tests for the pure utility/geometry helpers (Plan 00).
#include <doctest/doctest.h>

#include "Template/Point.h"
#include "Template/Range.h"
#include "Template/Rectangle.h"
#include "Util/QuadTree.h"
#include "Util/Randomizer.h"
#include "Util/TimedBool.h"

#include <cstdint>

using namespace jrc;

TEST_CASE("Range geometry")
{
    Range<int> r(2, 8);
    CHECK(r.length() == 6);
    CHECK(r.center() == 5);
    CHECK(r.contains(2));
    CHECK(r.contains(8));
    CHECK_FALSE(r.contains(9));
    CHECK(r.overlaps(Range<int>(7, 12)));
    CHECK_FALSE(r.overlaps(Range<int>(20, 30)));

    auto sym = Range<int>::symmetric(10, 3);
    CHECK(sym.first() == 7);
    CHECK(sym.second() == 13);
}

TEST_CASE("Rectangle contains and overlaps")
{
    Rectangle<int16_t> rect(0, 10, 0, 10); // l, r, t, b

    CHECK(rect.width() == 10);
    CHECK(rect.height() == 10);
    CHECK(rect.contains(Point<int16_t>(5, 5)));
    CHECK(rect.contains(Point<int16_t>(0, 0)));   // edges included
    CHECK(rect.contains(Point<int16_t>(10, 10)));
    CHECK_FALSE(rect.contains(Point<int16_t>(11, 5)));
    CHECK_FALSE(rect.contains(Point<int16_t>(-1, 5)));

    CHECK(rect.overlaps(Rectangle<int16_t>(5, 15, 5, 15)));
    CHECK_FALSE(rect.overlaps(Rectangle<int16_t>(20, 30, 20, 30)));

    SUBCASE("degenerate rectangle contains nothing")
    {
        Rectangle<int16_t> point_rect(5, 5, 5, 5);
        CHECK(point_rect.straight());
        CHECK_FALSE(point_rect.contains(Point<int16_t>(5, 5)));
    }
}

TEST_CASE("QuadTree stores and retrieves values")
{
    // A 1-D comparator is enough to exercise insert/lookup bookkeeping.
    auto cmp = [](const int& a, const int& b) {
        return a < b ? QuadTree<int, int>::LEFT : QuadTree<int, int>::RIGHT;
    };
    QuadTree<int, int> tree(cmp);

    tree.add(1, 100);
    tree.add(2, 200);
    tree.add(3, 50);

    CHECK(tree[1] == 100);
    CHECK(tree[2] == 200);
    CHECK(tree[3] == 50);

    SUBCASE("findnode returns 0 on an empty tree")
    {
        QuadTree<int, int> empty(cmp);
        auto always = [](const int&, const int&) { return true; };
        CHECK(empty.findnode(123, always) == 0);
    }

    SUBCASE("clear empties the tree")
    {
        tree.clear();
        auto always = [](const int&, const int&) { return true; };
        CHECK(tree.findnode(100, always) == 0);
    }
}

TEST_CASE("Randomizer stays within the requested bounds")
{
    Randomizer rng;

    for (int i = 0; i < 1000; ++i)
    {
        int v = rng.next_int(5, 10); // [from, to-1]
        CHECK(v >= 5);
        CHECK(v <= 9);

        int w = rng.next_int(4); // [0, to-1]
        CHECK(w >= 0);
        CHECK(w <= 3);

        float f = rng.next_real(2.0f, 3.0f); // [from, to)
        CHECK(f >= 2.0f);
        CHECK(f < 3.0f);
    }

    // Degenerate range returns the lower bound.
    CHECK(rng.next_int(7, 7) == 7);
    // A probability of 0 is never below; a probability of 1 is never above.
    CHECK_FALSE(rng.below(0.0f));
    CHECK_FALSE(rng.above(1.0f));
}

TEST_CASE("TimedBool decays after its delay elapses")
{
    TimedBool flag;
    CHECK_FALSE(static_cast<bool>(flag));

    flag.set_for(100);
    CHECK(static_cast<bool>(flag));

    flag.update(50);
    CHECK(static_cast<bool>(flag)); // 50 remaining

    flag.update(50);
    CHECK_FALSE(static_cast<bool>(flag)); // elapsed

    SUBCASE("direct assignment clears immediately")
    {
        TimedBool other;
        other.set_for(1000);
        other = false;
        CHECK_FALSE(static_cast<bool>(other));
    }
}
