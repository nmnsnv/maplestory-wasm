// Phase D: verify synthetic NX images built in code load back through the real
// nl::nx reader via nl::file::open_memory(). This is the asset mechanism the
// window render-list tests rely on (no committed binary fixture).
#include <doctest/doctest.h>

#include "Test/NxBuilder.h"

#include "nlnx/file.hpp"
#include "nlnx/node.hpp"
#include "nlnx/bitmap.hpp"

#include <string>

using jrc::test::NxBuilder;

TEST_CASE("Synthetic NX image round-trips through open_memory")
{
    NxBuilder builder;
    auto root = builder.root();

    builder.add_integer(root, "count", 42);
    builder.add_string(root, "title", "Hello");
    builder.add_vector(root, "origin", 3, 7);

    auto bytes = builder.build();

    nl::file file;
    file.open_memory(bytes.data(), bytes.size());
    nl::node r = file.root();

    SUBCASE("root sees all children")
    {
        CHECK(r.size() == 3);
    }

    SUBCASE("integer value")
    {
        CHECK(r["count"].get_integer() == 42);
    }

    SUBCASE("string value")
    {
        CHECK(r["title"].get_string() == "Hello");
    }

    SUBCASE("vector value")
    {
        CHECK(r["origin"].x() == 3);
        CHECK(r["origin"].y() == 7);
    }

    SUBCASE("missing child yields a null node with defaults")
    {
        nl::node missing = r["does_not_exist"];
        CHECK_FALSE(static_cast<bool>(missing));
        CHECK(missing.get_integer(99) == 99);
    }
}

TEST_CASE("Synthetic NX supports nested children")
{
    NxBuilder builder;
    auto root = builder.root();

    auto window = builder.add_child(root, "Login.img");
    builder.add_integer(window, "x", 100);
    auto nested = builder.add_child(window, "btLogin");
    builder.add_vector(nested, "origin", 5, 6);

    auto bytes = builder.build();

    nl::file file;
    file.open_memory(bytes.data(), bytes.size());
    nl::node r = file.root();

    CHECK(r["Login.img"]["x"].get_integer() == 100);
    CHECK(r["Login.img"]["btLogin"]["origin"].x() == 5);
    CHECK(r["Login.img"]["btLogin"]["origin"].y() == 6);

    // Path resolution should reach the same node.
    CHECK(r.resolve("Login.img/btLogin/origin").y() == 6);
}

TEST_CASE("Synthetic NX exposes bitmap dimensions and data")
{
    NxBuilder builder;
    auto root = builder.root();
    builder.add_bitmap(root, "icon", 16, 24);

    auto bytes = builder.build();

    nl::file file;
    file.open_memory(bytes.data(), bytes.size());
    nl::node r = file.root();

    nl::node icon = r["icon"];
    CHECK(icon.data_type() == nl::node::type::bitmap);

    nl::bitmap bmp = icon.get_bitmap();
    CHECK(bmp.width() == 16);
    CHECK(bmp.height() == 24);
    CHECK(bmp.length() == 4u * 16 * 24);
    // Pixel data decompresses without error (content is zeroed).
    CHECK(bmp.data() != nullptr);
}
