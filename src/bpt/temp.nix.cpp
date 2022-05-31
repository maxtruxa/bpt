#include "./temp.hpp"

#ifndef _WIN32

#include <vector>

using namespace bpt;

namespace {

auto string_to_vector(std::string const& s)
{
    std::vector<char> v(s.size() + 1);
    std::copy_n(s.c_str(), s.size() + 1, v.begin());
    return v;
}

} // namespace

temporary_dir temporary_dir::create_in(path_ref base) {
    auto file = (base / "bpt-tmp-XXXXXX").string();

    // mkdtemp() modifies the input buffer.
    auto templ = string_to_vector(file);
    const char* tempdir_path = ::mkdtemp(templ.data());

    if (tempdir_path == nullptr) {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "Failed to create a temporary directory");
    }
    auto path = fs::path(tempdir_path);
    return std::make_shared<impl>(std::move(path));
}

#endif
