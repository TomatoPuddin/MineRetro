#pragma once

#include <variant>
#include <cstdint>
#include <format>
#include <string>
#include <list>
#include <sstream>
#include <source_location>

#define MR_CHECK(__result, __code, ...) \
    if (auto&& __ref = (__result); !(__ref)) [[unlikely]] { \
        return {__ref, mineretro::ErrorCode::__code, #__code, std::source_location::current() __VA_OPT__(,) __VA_ARGS__ }; \
    }
#define MR_ASSERT(__exp, __code, ...) \
    if (!(__exp)) [[unlikely]] { \
        return {mineretro::ErrorCode::__code, #__code, std::source_location::current() __VA_OPT__(,) __VA_ARGS__ }; \
    }
#define MR_ERROR(__code, ...) \
    { \
       return {mineretro::ErrorCode::__code, #__code, std::source_location::current() __VA_OPT__(,) __VA_ARGS__ }; \
    }

namespace mineretro {
    namespace detail {
        struct Empty {};
    }

    enum class ErrorCode : uint32_t {
        Undefined = 0,

        ReadFile,
        WriteFile,
        FindSymbol,
        LoadLib,
        LoadRetroCoreLib,

        LastValue
    };

    struct Error {
        ErrorCode code = ErrorCode::Undefined;
        std::string_view type;
        std::source_location loc;
        std::string msg;

        [[nodiscard]] std::string describe() const {
            if (!msg.empty()) {
                return std::format("Err{} \"{}\" at [{}] ({}:{})",
                    type, msg, loc.function_name(), loc.file_name(), loc.line());
            }
            return std::format("Err{} at [{}] ({}:{})",
                type, loc.function_name(), loc.file_name(), loc.line());
        }
    };

    template <typename T> requires (!std::is_rvalue_reference_v<T>)
    class Result {
    public:
        static constexpr bool IsVoid = std::is_void_v<T>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;
        using ValueType = std::conditional_t<IsVoid, detail::Empty, std::remove_reference_t<T>>;
        using ErrorStack = std::list<Error>;

        template <typename = void> requires IsReference
        constexpr Result(ValueType& value) : data(std::ref(value)) {}
        template <typename Ref> requires (!IsVoid && !IsReference && std::is_same_v<std::remove_reference_t<Ref>, ValueType>)
        constexpr Result(Ref&& value) : data(std::move(value)) {}   // 左值也要移动
        template <typename...Args> requires (!IsReference && ((!std::is_same_v<std::decay_t<Args>, ErrorCode>)&&...))
        constexpr Result(Args&&...args) : data(std::in_place_type_t<ValueType>(), std::forward<Args>(args)...) {}

        Result(ErrorCode code, std::string_view type, std::source_location loc, std::string msg = "")
                : data(std::in_place_type_t<ErrorStack>()) {
            GetErrStack().emplace_front(code, type, std::move(loc), std::move(msg));
        }
        template <typename Other>
        Result(Result<Other> &last, ErrorCode code, std::string_view type, std::source_location loc, std::string msg = "")
                : data(std::move(last.GetErrStack())) {
            GetErrStack().emplace_front(code, type, std::move(loc), std::move(msg));
        }

        [[nodiscard]] bool IsOk() const noexcept { return std::holds_alternative<ValueType>(data); }
        [[nodiscard]] bool IsErr() const noexcept { return std::holds_alternative<ErrorStack>(data); }

        operator bool() const noexcept { return IsOk(); }
        ValueType& operator*() { return GetValue(); }

        [[nodiscard]] ValueType& GetValue() {
            if constexpr (IsReference) {
                return std::get<ValueType>(data).get();
            } else {
                return std::get<ValueType>(data);
            }
        }
        [[nodiscard]] const Error& GetErr() const { return std::get<ErrorStack>(data).first; }
        [[nodiscard]] const ErrorStack& GetErrStack() const { return std::get<ErrorStack>(data); }
        ErrorStack& GetErrStack() { return std::get<ErrorStack>(data); }

         [[nodiscard]] std::string describe() const {
            std::ostringstream stream;

            auto &stack = GetErrStack();
            auto iter = stack.begin();
            stream << iter->describe();
            while (++iter != stack.end()) {
                stream << "\n    " << iter->describe();
            }

            return stream.str();
        }

    private:
        std::variant<std::conditional_t<IsReference, std::reference_wrapper<ValueType>, ValueType>, ErrorStack> data;
    };

    constexpr Result<void> kSuccess;
}