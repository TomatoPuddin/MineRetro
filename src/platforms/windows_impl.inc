#pragma once

#include <windows.h>

#include "../platform.h"

namespace mineretro {
    namespace windows {
        class WindowsLibrary final : public Library {
        public:
            explicit WindowsLibrary(const HMODULE module) : module(module) {}

            Result<void*> FindSymbolAddr(const char* name) override {
                auto result = reinterpret_cast<void*>(GetProcAddress(module, name));
                MR_ASSERT(result != nullptr, FindSymbol, name)
                return result;
            }

            ~WindowsLibrary() override {
                FreeLibrary(module);
            }

        private:
            const HMODULE module;
        };
        class WindowsPlatform final : public Platform {
        public:
            Result<std::unique_ptr<Library>> LoadLib(const char* path) noexcept override {
                auto lib = LoadLibraryA(path);
                MR_ASSERT(lib != nullptr, LoadLib, path)
                return std::make_unique<WindowsLibrary>(lib);
            }
        };

        static WindowsPlatform instance;
    }

    Platform& Platform::get() {
        return windows::instance;
    }
}