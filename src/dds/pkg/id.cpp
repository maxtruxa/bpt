#include <dds/pkg/id.hpp>

#include <dds/error/errors.hpp>
#include <dds/util/result.hpp>

#include <fmt/core.h>

#include <tuple>

using namespace dds;

pkg_id pkg_id::parse(const std::string_view s) {
    DDS_E_SCOPE(e_invalid_pkg_id_str{std::string(s)});
    auto at_pos = s.find('@');
    if (at_pos == s.npos) {
        throw_user_error<errc::invalid_pkg_id>("Invalid package ID '{}'", s);
    }

    auto name    = s.substr(0, at_pos);
    auto ver_str = s.substr(at_pos + 1);

    try {
        return {std::string(name), semver::version::parse(ver_str)};
    } catch (const semver::invalid_version& err) {
        BOOST_LEAF_THROW_EXCEPTION(user_error<errc::invalid_pkg_id>("Package ID string is invalid"),
                                   err);
    }
}

pkg_id::pkg_id(std::string_view n, semver::version v)
    : name(n)
    , version(std::move(v)) {
    if (name.find('@') != name.npos) {
        throw_user_error<errc::invalid_pkg_name>(
            "Invalid package name '{}' (The '@' character is not allowed)");
    }
}

std::string pkg_id::to_string() const noexcept { return name + "@" + version.to_string(); }