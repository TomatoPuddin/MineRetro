#pragma once

#include <vector>
#include <span>
#include <array>

namespace mineretro {
    template<size_t Size>
    using BufferArray = std::array<std::byte, Size>;
    using Buffer = std::vector<std::byte>;
    using BufferView = std::span<std::byte>;
    using BufferViewR = std::span<const std::byte>;
}