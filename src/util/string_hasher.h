#pragma once

#include <string>

namespace mineretro {
    struct GenericStringHasher {
        size_t operator()(const std::string &key) const noexcept {
            return std::hash<std::string>()(key);
        }

        size_t operator()(std::string_view key) const noexcept {
            return std::hash<std::string_view>()(key);
        }

        size_t operator()(const char* key) const noexcept {
            return std::hash<std::string_view>()(key);
        }

        using is_transparent = int;
    };
}