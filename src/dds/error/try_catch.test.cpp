#include "./try_catch.hpp"

#include <catch2/catch.hpp>

TEST_CASE("Try-catch") {
    auto r = dds_leaf_try { return 2; }
    dds_leaf_catch_all->int { return 0; };
    CHECK(r == 2);
}