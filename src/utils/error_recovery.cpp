/**
 * @file error_recovery.cpp
 * @brief Error recovery implementation
 * @version 0.1.0
 */

#include <rangelua/utils/error_recovery.hpp>
#include <rangelua/utils/logger.hpp>

#include <thread>

namespace rangelua::utils {

    // Thread-local storage for error context stack
    thread_local std::vector<String> ErrorContext::context_stack_;

}  // namespace rangelua::utils
