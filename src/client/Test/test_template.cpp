// Characterization tests for the pure Template/ containers (Plan 00).
#include <doctest/doctest.h>

#include "Template/BoolPair.h"
#include "Template/EnumMap.h"
#include "Template/Interpolated.h"
#include "Template/Optional.h"

using namespace jrc;

namespace
{
    // EnumMap indexes a std::array by the enum, so it needs an unscoped enum
    // (implicitly convertible to size_t), matching how the client uses it.
    enum Color
    {
        RED,
        GREEN,
        BLUE,
        LENGTH
    };
}

TEST_CASE("EnumMap is addressable by enum and default-initialized")
{
    EnumMap<Color, int> map;

    // Value-initialized to int() == 0.
    CHECK(map[Color::RED] == 0);

    map[Color::GREEN] = 42;
    CHECK(map[Color::GREEN] == 42);

    CHECK(map.keys()[0] == Color::RED);
    CHECK(map.keys()[2] == Color::BLUE);

    SUBCASE("iteration visits every key")
    {
        int count = 0;
        for (auto it = map.begin(); it != map.end(); ++it)
        {
            ++count;
        }
        CHECK(count == 3);
    }

    SUBCASE("clear resets values")
    {
        map[Color::BLUE] = 7;
        map.clear();
        CHECK(map[Color::BLUE] == 0);
    }
}

TEST_CASE("Optional models a nullable pointer")
{
    Optional<int> empty;
    CHECK_FALSE(static_cast<bool>(empty));

    int value = 99;
    Optional<int> present(&value);
    CHECK(static_cast<bool>(present));
    CHECK(present.get() == &value);
    CHECK(*present == 99);

    *present = 5;
    CHECK(value == 5);
}

TEST_CASE("BoolPair indexes by boolean")
{
    BoolPair<int> pair(1, 2);
    CHECK(pair[true] == 1);
    CHECK(pair[false] == 2);

    pair.set(true, 9);
    CHECK(pair[true] == 9);
    CHECK(pair[false] == 2);
}

TEST_CASE("Nominal returns now/before based on the threshold")
{
    Nominal<int> nom;
    nom.set(5);
    CHECK(nom.get() == 5);

    nom.next(9, 0.5f);
    CHECK(nom.last() == 5);
    CHECK(nom.get() == 9);
    CHECK(nom.get(0.4f) == 5); // below threshold -> before
    CHECK(nom.get(0.6f) == 9); // at/above threshold -> now
}

TEST_CASE("Linear interpolates between before and now")
{
    Linear<int> lin;
    lin.set(10);
    lin = 20; // shifts before<-10, now<-20

    CHECK(lin.get() == 20);
    CHECK(lin.last() == 10);
    CHECK(lin.get(0.0f) == 10); // alpha 0 -> before
    CHECK(lin.get(1.0f) == 20); // alpha 1 -> now
}
