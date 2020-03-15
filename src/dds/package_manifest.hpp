#pragma once

#include <dds/deps.hpp>
#include <dds/util/fs.hpp>
#include <semver/version.hpp>

#include <optional>
#include <string>
#include <vector>

namespace dds {

enum class test_lib {
    catch_,
    catch_main,
    catch_runner,
};

struct package_manifest {
    std::string             name;
    std::string             namespace_;
    std::optional<test_lib> test_driver;
    semver::version         version;
    std::vector<dependency> dependencies;
    static package_manifest load_from_file(path_ref);
};

}  // namespace dds