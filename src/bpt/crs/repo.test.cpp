#include "./repo.hpp"

#include <bpt/bpt.test.hpp>
#include <bpt/crs/error.hpp>
#include <bpt/error/try_catch.hpp>
#include <bpt/temp.hpp>

#include <catch2/catch.hpp>
#include <neo/ranges.hpp>

#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("Init repo") {
    auto tempdir = bpt::temporary_dir::create();
    auto repo
        = REQUIRES_LEAF_NOFAIL(bpt::crs::repository::create(tempdir.path(), "simple-test-repo"));
    bpt_leaf_try {
        bpt::crs::repository::create(repo.root(), "test");
        FAIL("Expected an error, but no error occurred");
    }
    bpt_leaf_catch(bpt::crs::e_repo_already_init) {}
    bpt_leaf_catch_all { FAIL("Unexpected failure: " << diagnostic_info); };
    CHECK(repo.name() == "simple-test-repo");
}

struct empty_repo {
    bpt::temporary_dir   tempdir = bpt::temporary_dir::create();
    bpt::crs::repository repo    = bpt::crs::repository::create(tempdir.path(), "test");
};

TEST_CASE_METHOD(empty_repo, "Import a simple packages") {
    REQUIRES_LEAF_NOFAIL(repo.import_dir(bpt::testing::DATA_DIR / "simple.crs"));
    auto all = REQUIRES_LEAF_NOFAIL(repo.all_packages() | neo::to_vector);
    REQUIRE(all.size() == 1);
    auto first = all.front();
    CHECK(first.id.name.str == "test-pkg");
    CHECK(first.id.version.to_string() == "1.2.43");
    CHECKED_IF(fs::is_directory(repo.pkg_dir())) {
        CHECKED_IF(fs::is_directory(repo.pkg_dir() / "test-pkg")) {
            CHECKED_IF(fs::is_directory(repo.pkg_dir() / "test-pkg/1.2.43~1")) {
                CHECK(fs::is_regular_file(repo.pkg_dir() / "test-pkg/1.2.43~1/pkg.tgz"));
                CHECK(fs::is_regular_file(repo.pkg_dir() / "test-pkg/1.2.43~1/pkg.json"));
            }
        }
    }

    REQUIRES_LEAF_NOFAIL(repo.import_dir(bpt::testing::DATA_DIR / "simple2.crs"));
    all = REQUIRES_LEAF_NOFAIL(repo.all_packages() | neo::to_vector);
    REQUIRE(all.size() == 2);
    first       = all[0];
    auto second = all[1];
    CHECK(first.id.name.str == "test-pkg");
    CHECK(second.id.name.str == "test-pkg");
    CHECK(first.id.version.to_string() == "1.2.43");
    CHECK(second.id.version.to_string() == "1.3.0");

    REQUIRES_LEAF_NOFAIL(repo.import_dir(bpt::testing::DATA_DIR / "simple3.crs"));
    all = REQUIRES_LEAF_NOFAIL(repo.all_packages() | neo::to_vector);
    REQUIRE(all.size() == 3);
    auto third = all[2];
    CHECK(third.id.name.str == "test-pkg");
    CHECK(third.id.version.to_string() == "1.3.0");
    CHECK(third.id.revision == 2);
}
