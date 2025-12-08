#pragma once

#include <memory>

#include "error.h"

#ifdef MR_WINDOWS
#include "platforms/windows.h"
#else
#include "platforms/unix_like.h"
#endif

namespace mineretro {
    class Library {
    public:
        virtual Result<void*> FindSymbolAddr(const char* name) = 0;
        virtual ~Library() = default;

        template<auto Func>
        Result<decltype(Func)> FindSymbol(const char* name) {
            auto res = FindSymbolAddr(name);
            MR_CHECK(res, FindSymbol, name)
            return reinterpret_cast<decltype(Func)>(res.GetValue());
        }

        template<typename FuncType>
        Result<void> FindSymbol(const char* name, FuncType &func) {
            auto res = FindSymbolAddr(name);
            MR_CHECK(res, FindSymbol, name)
            func = reinterpret_cast<FuncType>(*res);
            return kSuccess;
        }
    };

    class Platform {
    public:
        virtual Result<std::unique_ptr<Library>> LoadLib(const char* path) = 0;
        virtual ~Platform() = default;

        static Platform& get();
    };
}
