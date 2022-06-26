#include "./file.hpp"

#include <bpt/util/string.hpp>

#include <algorithm>
#include <cassert>
#include <optional>
#include <vector>

using namespace bpt;

std::optional<source_kind> bpt::infer_source_kind(path_ref p) noexcept {
    static std::vector<std::string_view> header_exts = {
        ".H",
        ".H++",
        ".h",
        ".h++",
        ".hh",
        ".hpp",
        ".hxx",
    };
    static std::vector<std::string_view> header_impl_exts = {
        ".inc",
        ".inl",
        ".ipp",
    };
    static std::vector<std::string_view> source_exts = {
        ".C",
        ".c",
        ".c++",
        ".cc",
        ".cpp",
        ".cxx",
    };
    assert(std::is_sorted(header_exts.begin(), header_exts.end()));
    assert(std::is_sorted(header_impl_exts.begin(), header_impl_exts.end()));
    assert(std::is_sorted(source_exts.begin(), source_exts.end()));
    auto leaf = p.filename();

    const auto is_in = [](const auto& exts, const auto& ext) {
        auto ext_found = std::lower_bound(exts.begin(), exts.end(), ext, std::less<>());
        return ext_found != exts.end() && *ext_found == ext;
    };

    if (is_in(header_exts, p.extension())) {
        return source_kind::header;
    }

    if (is_in(header_impl_exts, p.extension())) {
        return source_kind::header_impl;
    }

    if (!is_in(source_exts, p.extension())) {
        return std::nullopt;
    }

    if (p.stem().extension() == ".test") {
        return source_kind::test;
    }

    if (p.stem().extension() == ".main") {
        return source_kind::app;
    }

    return source_kind::source;
}

std::optional<source_file> source_file::from_path(path_ref path, path_ref base_path) noexcept {
    auto kind = infer_source_kind(path);
    if (!kind.has_value()) {
        return std::nullopt;
    }

    return source_file{path, base_path, *kind};
}
