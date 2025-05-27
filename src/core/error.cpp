/**
 * @file error.cpp
 * @brief Error handling implementation with enhanced debugging support
 * @version 0.1.0
 */

#include <rangelua/core/config.hpp>
#include <rangelua/core/error.hpp>
#include <rangelua/utils/logger.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>

#ifdef __has_include
#    if __has_include(<execinfo.h>)
#        include <execinfo.h>
#        define RANGELUA_HAS_BACKTRACE 1
#    endif
#    if __has_include(<cxxabi.h>)
#        include <cxxabi.h>
#        define RANGELUA_HAS_DEMANGLE 1
#    endif
#endif

namespace rangelua {

    namespace {
        /**
         * @brief Generate stack trace for debugging
         */
        std::vector<String> generate_stack_trace() {
            std::vector<String> trace;

#ifdef RANGELUA_HAS_BACKTRACE
            constexpr int max_frames = config::MAX_STACK_TRACE_DEPTH;
            std::array<void*, max_frames> buffer{};

            int frame_count = backtrace(buffer.data(), max_frames);

            // Use unique_ptr for automatic memory management
            struct SymbolsDeleter {
                void operator()(char** ptr) const noexcept {
                    if (ptr) {
                        std::free(static_cast<void*>(ptr));
                    }
                }
            };

            std::unique_ptr<char*, SymbolsDeleter> symbols{
                backtrace_symbols(buffer.data(), frame_count)};

            if (symbols) {
                for (int i = 0; i < frame_count; ++i) {
                    String frame = symbols.get()[i];

#    ifdef RANGELUA_HAS_DEMANGLE
                    // Try to demangle C++ symbols
                    size_t start = frame.find('(');
                    size_t end = frame.find('+');

                    if (start != String::npos && end != String::npos && start < end) {
                        String mangled = frame.substr(start + 1, end - start - 1);

                        int status = 0;

                        // Use unique_ptr for automatic memory management
                        struct DemangledDeleter {
                            void operator()(char* ptr) const noexcept {
                                if (ptr) {
                                    std::free(ptr);
                                }
                            }
                        };

                        std::unique_ptr<char, DemangledDeleter> demangled{
                            abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status)};

                        if (status == 0 && demangled) {
                            frame =
                                frame.substr(0, start + 1) + demangled.get() + frame.substr(end);
                        }
                    }
#    endif
                    trace.push_back(std::move(frame));
                }
            }
#else
            trace.push_back("Stack trace not available on this platform");
#endif

            return trace;
        }

        /**
         * @brief Format source location for display
         */
        [[maybe_unused]] String format_source_location(const SourceLocation& loc) {
            if (loc.filename_.empty()) {
                return "<unknown>";
            }

            std::ostringstream oss;
            oss << loc.filename_;
            if (loc.line_ > 0) {
                oss << ":" << loc.line_;
                if (loc.column_ > 0) {
                    oss << ":" << loc.column_;
                }
            }
            return oss.str();
        }
    }  // namespace

    String Exception::detailed_message() const {
        std::ostringstream oss;

        // Header
        oss << "┌─ RangeLua Exception ─────────────────────────────────────────\n";

        // Basic information
        oss << "│ Type: " << ::rangelua::error_code_to_string(code_) << "\n";
        oss << "│ Message: " << message_ << "\n";

        // Source location
        oss << "│ Source: " << location_.file_name() << ":" << location_.line() << "\n";
        oss << "│ Function: " << location_.function_name() << "\n";

        // Stack trace
        if constexpr (config::DEBUG_ENABLED) {
            auto trace = generate_stack_trace();
            if (!trace.empty()) {
                oss << "│\n│ Stack Trace:\n";
                for (size_t i = 0; i < std::min(trace.size(), static_cast<size_t>(10)); ++i) {
                    oss << "│   " << std::setw(2) << i << ": " << trace[i] << "\n";
                }
                if (trace.size() > 10) {
                    oss << "│   ... (" << (trace.size() - 10) << " more frames)\n";
                }
            }
        }

        oss << "└─────────────────────────────────────────────────────────────";

        return oss.str();
    }

    String ErrorCategory::message(int ev) const {
        switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::SUCCESS:
                return "Operation completed successfully";
            case ErrorCode::SYNTAX_ERROR:
                return "Syntax error in Lua code";
            case ErrorCode::RUNTIME_ERROR:
                return "Runtime error during execution";
            case ErrorCode::MEMORY_ERROR:
                return "Memory allocation or management error";
            case ErrorCode::TYPE_ERROR:
                return "Type mismatch or invalid type operation";
            case ErrorCode::ARGUMENT_ERROR:
                return "Invalid function argument";
            case ErrorCode::STACK_OVERFLOW:
                return "Stack overflow detected";
            case ErrorCode::COROUTINE_ERROR:
                return "Coroutine operation error";
            case ErrorCode::IO_ERROR:
                return "Input/output operation error";
            case ErrorCode::UNKNOWN_ERROR:
            default:
                return "Unknown or unspecified error";
        }
    }

    void log_error(const Exception& ex) {
        if (auto logger = utils::loggers::memory()) {
            logger->error("Exception occurred: {}", ex.detailed_message());
        }
    }

    void log_error(ErrorCode code, StringView message) {
        if (auto logger = utils::loggers::memory()) {
            logger->error("Error [{}]: {}", ::rangelua::error_code_to_string(code), message);
        }
    }

    String format_error_message(ErrorCode code, StringView context) {
        std::ostringstream oss;
        oss << "[" << ::rangelua::error_code_to_string(code) << "]";

        if (!context.empty()) {
            oss << " in " << context;
        }

        oss << ": " << ErrorCategory::instance().message(static_cast<int>(code));

        return oss.str();
    }

    String format_exception_details(const Exception& ex) {
        std::ostringstream oss;

        oss << "Exception Details:\n";
        oss << "  Type: " << ::rangelua::error_code_to_string(ex.code()) << "\n";
        oss << "  Message: " << ex.what() << "\n";
        oss << "  Location: " << ex.location().file_name() << ":" << ex.location().line() << "\n";
        oss << "  Function: " << ex.location().function_name() << "\n";

        return oss.str();
    }

    // Export error_code_to_string for external use
    StringView error_code_to_string(ErrorCode code) noexcept {
        // Call the anonymous namespace function directly
        switch (code) {
            case ErrorCode::SUCCESS:
                return "SUCCESS";
            case ErrorCode::SYNTAX_ERROR:
                return "SYNTAX_ERROR";
            case ErrorCode::RUNTIME_ERROR:
                return "RUNTIME_ERROR";
            case ErrorCode::MEMORY_ERROR:
                return "MEMORY_ERROR";
            case ErrorCode::TYPE_ERROR:
                return "TYPE_ERROR";
            case ErrorCode::ARGUMENT_ERROR:
                return "ARGUMENT_ERROR";
            case ErrorCode::STACK_OVERFLOW:
                return "STACK_OVERFLOW";
            case ErrorCode::COROUTINE_ERROR:
                return "COROUTINE_ERROR";
            case ErrorCode::IO_ERROR:
                return "IO_ERROR";
            case ErrorCode::UNKNOWN_ERROR:
                return "UNKNOWN_ERROR";
            default:
                return "INVALID_ERROR_CODE";
        }
    }

}  // namespace rangelua
