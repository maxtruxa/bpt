#pragma once

#include <bpt/error/result_fwd.hpp>

#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace bpt {

struct e_open_file_path {
    std::filesystem::path value;
};

struct e_write_file_path {
    std::filesystem::path value;
};

struct e_read_file_path {
    std::filesystem::path value;
};

[[nodiscard]] std::fstream open_file(std::filesystem::path const& filepath, std::ios::openmode);
void                       write_file(std::filesystem::path const& path, std::string_view);
[[nodiscard]] std::string  read_file(std::filesystem::path const& path);

}  // namespace bpt
