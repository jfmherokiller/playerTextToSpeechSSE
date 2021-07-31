#pragma once
#include <boost/algorithm/string.hpp>
#include <string>
#include <sapi.h>
#include <comdef.h>
#include <sstream>
#include <cmath>
#undef _UNICODE
#include <sphelper.h>
#define _UNICODE
#undef IMAGE_SCN_MEM_EXECUTE
#undef IMAGE_SCN_MEM_WRITE
#undef INVALID_HANDLE_VALUE
#undef MAX_PATH
#undef MEM_RELEASE
#include "xbyak/xbyak.h"
#undef PAGE_EXECUTE_READWRITE
#undef GetModuleFileName
#undef GetEnvironmentVariable
#undef MessageBox
#undef VerQueryValue
#undef GetFileVersionInfo
#undef GetFileVersionInfoSize
#undef GetModuleHandle
#undef max
#undef min
#undef GetObject
#ifndef __cpp_consteval
namespace std {
    inline namespace fundamentals_v2 {
        struct source_location
        {
        private:
            using uint_least32_t = unsigned;
        public:

            // 14.1.2, source_location creation
            static constexpr source_location
            current(const char* __file = __builtin_FILE(),
                    const char* __func = __builtin_FUNCTION(),
                    int __line = __builtin_LINE(),
                    int __col = 0) noexcept
            {
                source_location __loc;
                __loc._M_file = __file;
                __loc._M_func = __func;
                __loc._M_line = __line;
                __loc._M_col = __col;
                return __loc;
            }

            constexpr source_location() noexcept
                : _M_file("unknown"), _M_func(_M_file), _M_line(0), _M_col(0)
            { }

            // 14.1.3, source_location field access
            [[nodiscard]] constexpr uint_least32_t line() const noexcept { return _M_line; }
            [[nodiscard]] constexpr uint_least32_t column() const noexcept { return _M_col; }
            [[nodiscard]] constexpr const char* file_name() const noexcept { return _M_file; }
            [[nodiscard]] constexpr const char* function_name() const noexcept { return _M_func; }

        private:
            const char* _M_file;
            const char* _M_func;
            uint_least32_t _M_line;
            uint_least32_t _M_col;
        };
    } // namespace fundamentals_v2
} // namespace std
#endif
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include "SpeechStuff.h"

#pragma warning(push)
#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

namespace logger = SKSE::log;

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
