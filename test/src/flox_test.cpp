#include <catch2/catch_test_macros.hpp>

#include <gfx/gfx.hpp>

TEST_CASE("Can initialize", "[gfx]") {
    gfx::gfx gfx{};
    REQUIRE(gfx.is_initialized());
}

TEST_CASE("Can create window", "[gfx]") {
    gfx::gfx gfx{};
    REQUIRE_NOTHROW(gfx.create_window("Can create window", 100, 100));
}
