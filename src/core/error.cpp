/**
 * @file error.cpp
 * @brief Error handling implementation
 * @version 0.1.0
 */

#include <rangelua/core/error.hpp>

#include <sstream>

namespace rangelua {

    String Exception::detailed_message() const {
        std::ostringstream oss;
        oss << "Exception: " << message_ << "\n";
        oss << "Error Code: " << static_cast<int>(code_) << "\n";
        oss << "Location: " << location_.file_name() << ":" << location_.line() << "\n";
        oss << "Function: " << location_.function_name() << "\n";

        return oss.str();
    }

    String ErrorCategory::message(int ev) const {
        switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::SUCCESS:
                return "Success";
            case ErrorCode::SYNTAX_ERROR:
                return "Syntax error";
            case ErrorCode::RUNTIME_ERROR:
                return "Runtime error";
            case ErrorCode::MEMORY_ERROR:
                return "Memory error";
            case ErrorCode::TYPE_ERROR:
                return "Type error";
            case ErrorCode::ARGUMENT_ERROR:
                return "Argument error";
            case ErrorCode::STACK_OVERFLOW:
                return "Stack overflow";
            case ErrorCode::COROUTINE_ERROR:
                return "Coroutine error";
            case ErrorCode::IO_ERROR:
                return "I/O error";
            case ErrorCode::UNKNOWN_ERROR:
            default:
                return "Unknown error";
        }
    }

}  // namespace rangelua
