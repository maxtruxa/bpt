#include "./toolchain.hpp"

#include <dds/lm_parse.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace dds;

using std::optional;
using std::string;
using std::string_view;
using std::vector;
using opt_string = optional<string>;

namespace {

struct invalid_toolchain : std::runtime_error {
    using std::runtime_error::runtime_error;
};

}  // namespace

toolchain toolchain::load_from_file(fs::path p) {
    opt_string inc_template;
    opt_string def_template;

    opt_string c_compile_template;
    opt_string cxx_compile_template;
    opt_string create_archive_template;
    opt_string warning_flags;

    opt_string archive_suffix;

    auto require_key = [](auto k, auto& opt) {
        if (!opt.has_value()) {
            throw invalid_toolchain("Toolchain file is missing a required key: " + string(k));
        }
    };

    auto kvs = lm_parse_file(p);
    for (auto&& pair : kvs.items()) {
        auto& key   = pair.key();
        auto& value = pair.value();

        auto try_single = [&](auto k, auto& opt) {
            if (key == k) {
                if (opt.has_value()) {
                    throw invalid_toolchain("Duplicate key: " + key);
                }
                opt = value;
                return true;
            }
            return false;
        };

        // clang-format off
        bool found_single = false // Bool to force alignment
            // Argument templates
            || try_single("Include-Template", inc_template)
            || try_single("Define-Template", def_template)
            // Command templates
            || try_single("Compile-C-Template", c_compile_template)
            || try_single("Compile-C++-Template", cxx_compile_template)
            || try_single("Create-Archive-Template", create_archive_template)
            || try_single("Archive-Suffix", archive_suffix)
            || try_single("Warning-Flags", warning_flags)
            || false;
        // clang-format on

        if (!found_single) {
            throw invalid_toolchain("Unknown toolchain file key: " + key);
        }
    }

    require_key("Include-Template", inc_template);
    require_key("Define-Template", def_template);

    require_key("Compile-C-Template", c_compile_template);
    require_key("Compile-C++-Template", cxx_compile_template);
    require_key("Create-Archive-Template", create_archive_template);

    require_key("Archive-Suffix", archive_suffix);

    return toolchain{
        c_compile_template.value(),
        cxx_compile_template.value(),
        inc_template.value(),
        def_template.value(),
        create_archive_template.value(),
        archive_suffix.value(),
        warning_flags.value_or(""),
    };
}

vector<string> dds::split_shell_string(std::string_view shell) {
    char cur_quote  = 0;
    bool is_escaped = false;

    vector<string> acc;

    const auto begin = shell.begin();
    auto       iter  = begin;
    const auto end   = shell.end();

    opt_string token;
    while (iter != end) {
        const char c = *iter++;
        if (is_escaped) {
            if (c == '\n') {
                // Ignore the newline
            } else if (cur_quote || c != cur_quote || c == '\\') {
                // Escaped `\` character
                token = token.value_or("") + c;
            } else {
                // Regular escape sequence
                token = token.value_or("") + '\\' + c;
            }
            is_escaped = false;
        } else if (c == '\\') {
            is_escaped = true;
        } else if (cur_quote) {
            if (c == cur_quote) {
                // End of quoted token;
                cur_quote = 0;
            } else {
                token = token.value_or("") + c;
            }
        } else if (c == '"' || c == '\'') {
            // Beginning of a quoted token
            cur_quote = c;
            token     = "";
        } else if (c == '\t' || c == ' ' || c == '\n' || c == '\r' || c == '\f') {
            // We've reached unquoted whitespace
            if (token.has_value()) {
                acc.push_back(move(*token));
            }
            token.reset();
        } else {
            // Just a regular character
            token = token.value_or("") + c;
        }
    }

    if (token.has_value()) {
        acc.push_back(move(*token));
    }

    return acc;
}

namespace {

std::string replace(std::string_view str, std::string_view key, std::string_view repl) {
    std::string                 ret;
    std::string_view::size_type pos      = 0;
    std::string_view::size_type prev_pos = 0;
    while (pos = str.find(key, pos), pos != key.npos) {
        ret.append(str.begin() + prev_pos, str.begin() + pos);
        ret.append(repl);
        prev_pos = pos += key.size();
    }
    ret.append(str.begin() + prev_pos, str.end());
    return ret;
}

vector<string> replace(vector<string> strings, std::string_view key, std::string_view repl) {
    for (auto& item : strings) {
        item = replace(item, key, repl);
    }
    return strings;
}
}  // namespace

vector<string> toolchain::include_args(const fs::path& p) const noexcept {
    return replace(_inc_template, "<PATH>", p.string());
}

vector<string> toolchain::definition_args(std::string_view s) const noexcept {
    return replace(_def_template, "<DEF>", s);
}

vector<string> toolchain::create_compile_command(const compile_file_spec& spec) const noexcept {
    vector<string> flags;

    for (auto&& inc_dir : spec.include_dirs) {
        auto inc_args = include_args(inc_dir);
        extend(flags, inc_args);
    }

    for (auto&& def : spec.definitions) {
        auto def_args = definition_args(def);
        extend(flags, def_args);
    }

    if (spec.enable_warnings) {
        extend(flags, _warning_flags);
    }

    vector<string> command;
    for (auto arg : _cxx_compile) {
        if (arg == "<FLAGS>") {
            extend(command, flags);
        } else {
            arg = replace(arg, "<FILE>", spec.source_path.string());
            arg = replace(arg, "<OUT>", spec.out_path.string());
            command.push_back(arg);
        }
    }
    return command;
}

vector<string> toolchain::create_archive_command(const archive_spec& spec) const noexcept {
    vector<string> cmd;
    for (auto& arg : _archive_template) {
        if (arg == "<OBJECTS>") {
            extend(cmd, spec.input_files);
        } else {
            cmd.push_back(replace(arg, "<ARCHIVE>", spec.out_path.string()));
        }
    }
    return cmd;
}