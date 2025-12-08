#include "file.h"

#include <fstream>

namespace mineretro {
    constexpr size_t MAX_FILE_SIZE = 66 * 1024 * 1024;

    Result<void> writeFile(const std::filesystem::path& path, BufferViewR fileBuffer) {
        MR_ASSERT(fileBuffer.size() <= MAX_FILE_SIZE, WriteFile);

        std::ofstream stream;
        stream.exceptions(std::ios_base::goodbit);

        std::error_code ec;
        if (path.has_parent_path()) {
            MR_ASSERT(std::filesystem::create_directories(path.parent_path(), ec), WriteFile, "Failed to create directory");
        }
        stream.open(path, std::ios_base::binary | std::ios_base::trunc);
        MR_ASSERT(stream.good(), WriteFile, "Failed to open file");

        stream.write(reinterpret_cast<const char *>(fileBuffer.data()), static_cast<std::streamsize>(fileBuffer.size()));
        MR_ASSERT(stream.good(), WriteFile, "Failed to write data");

        stream.flush();
        MR_ASSERT(stream.good(), WriteFile, "Failed to flush");

        stream.close();
        return kSuccess;
    }

    Result<void> readFile(const std::filesystem::path& path, Buffer &fileBuffer) {
        std::error_code ec;
        auto fileSize = std::filesystem::file_size(path, ec);
        MR_ASSERT(!ec, ReadFile, "Failed to retrieve file size")
        MR_ASSERT(fileSize < MAX_FILE_SIZE, ReadFile, "File size too large")

        std::ifstream stream;
        stream.exceptions(std::ios_base::goodbit);

        stream.open(path, std::ios_base::binary);
        MR_ASSERT(stream.good(), ReadFile, "Failed to open file");

        fileBuffer.resize(fileSize);
        std::streamsize read = 0;
        while (read < fileSize) {
            stream.read(reinterpret_cast<char *>(fileBuffer.data() + read), fileSize - read);
            MR_ASSERT(stream.good(), ReadFile, "Failed to read file data");
            read += stream.gcount();
        }

        stream.close();
        return kSuccess;
    }
}