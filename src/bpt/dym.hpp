#pragma once

#include <range/v3/algorithm/min_element.hpp>
#include <range/v3/view/all.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace dds {

std::size_t lev_edit_distance(std::string_view a, std::string_view b) noexcept;

template <typename Range>
std::optional<std::string> did_you_mean(std::string_view given, Range&& strings) noexcept {
    auto cand = ranges::min_element(strings, ranges::less{}, [&](std::string_view candidate) {
        return lev_edit_distance(candidate, given);
    });
    if (cand == ranges::end(strings)) {
        return std::nullopt;
    } else {
        return std::string(*cand);
    }
}

inline std::optional<std::string>
did_you_mean(std::string_view given, std::initializer_list<std::string_view> strings) noexcept {
    return did_you_mean(given, ranges::views::all(strings));
}

}  // namespace dds