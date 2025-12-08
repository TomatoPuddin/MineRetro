#pragma once

#include <filesystem>
#include "buffer.h"
#include "../error.h"

namespace mineretro {
    Result<void> writeFile(const std::filesystem::path& path, BufferViewR fileBuffer);

    Result<void> readFile(const std::filesystem::path& path, Buffer &fileBuffer);

    inline Result<Buffer> readFile(const std::filesystem::path& path) {
        Buffer buffer;
        MR_CHECK(readFile(path, buffer), ReadFile);
        return buffer;
    }
}