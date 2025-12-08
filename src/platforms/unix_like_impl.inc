#pragma once

#include <dlfcn.h>

#include "../platform.h"

namespace mineretro {
    namespace unixlike {
        class UnixLikeLibrary final : public Library {
        public:
            explicit UnixLikeLibrary(void* handle) : handle(handle) {}

            Result<void*> FindSymbolAddr(const char* name) override {
                auto res = dlsym(handle, name);
                MR_ASSERT(res != nullptr, FindSymbol, name)
                return res;
            }

            ~UnixLikeLibrary() override {
                dlclose(handle);
            }

        private:
            void* const handle;
        };
        class UnixLikePlatform final : public Platform {
        public:
            Result<std::unique_ptr<Library>> LoadLib(const char* path) noexcept override {
                auto lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
                MR_ASSERT(lib != nullptr, LoadLib, path)
                return std::make_unique<UnixLikeLibrary>(lib);
            }
        };

        static UnixLikePlatform instance;
    }

    Platform& Platform::get() {
        return unixlike::instance;
    }
}