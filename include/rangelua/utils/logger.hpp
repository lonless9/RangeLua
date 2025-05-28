#pragma once

/**
 * @file logger.hpp
 * @brief Enhanced logging system with spdlog integration and module-specific control
 * @version 0.1.0
 */

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/config.hpp"
#include "../core/types.hpp"

namespace rangelua::utils {

    /**
     * @brief Enhanced logger wrapper around spdlog with module-specific control
     */
    class Logger {
    public:
        using LogLevel = spdlog::level::level_enum;

        /**
         * @brief Initialize logger system
         */
        static void initialize(const String& name = "rangelua", LogLevel level = LogLevel::info);

        /**
         * @brief Shutdown logger system
         */
        static void shutdown();

        /**
         * @brief Get default logger
         */
        static std::shared_ptr<spdlog::logger> get_logger();

        /**
         * @brief Create module-specific logger
         */
        static std::shared_ptr<spdlog::logger> create_logger(const String& module_name);

        /**
         * @brief Set global log level
         */
        static void set_level(LogLevel level);

        /**
         * @brief Set log level for specific module
         */
        static void set_module_level(const String& module_name, LogLevel level);

        /**
         * @brief Get log level for specific module
         */
        static LogLevel get_module_level(const String& module_name);

        /**
         * @brief Add file sink
         */
        static void add_file_sink(const String& filename);

        /**
         * @brief Set log pattern
         */
        static void set_pattern(const String& pattern);

        /**
         * @brief Parse and apply module-specific log levels from command line arguments
         * Format: "module1:level1,module2:level2" or "level" for global
         */
        static void configure_from_args(const std::vector<std::string>& log_configs);

        /**
         * @brief Convert string to log level
         */
        static LogLevel string_to_log_level(const std::string& level);

        /**
         * @brief Convert log level to string
         */
        static std::string log_level_to_string(LogLevel level);

        /**
         * @brief Get list of available modules
         */
        static std::vector<std::string> get_available_modules();

        /**
         * @brief Enable logging for all available modules at the specified level
         */
        static void enable_all_modules(LogLevel level);

    private:
        static std::shared_ptr<spdlog::logger> default_logger_;
        static std::vector<spdlog::sink_ptr> sinks_;
        static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> module_loggers_;
        static std::unordered_map<std::string, LogLevel> module_levels_;
        static bool initialized_;
    };

    /**
     * @brief Module-specific loggers
     */
    namespace loggers {

        std::shared_ptr<spdlog::logger> lexer();
        std::shared_ptr<spdlog::logger> parser();
        std::shared_ptr<spdlog::logger> codegen();
        std::shared_ptr<spdlog::logger> optimizer();
        std::shared_ptr<spdlog::logger> vm();
        std::shared_ptr<spdlog::logger> memory();
        std::shared_ptr<spdlog::logger> gc();

    }  // namespace loggers

}  // namespace rangelua::utils

// Convenience macros for logging
#define RANGELUA_LOG_TRACE(logger, ...)    SPDLOG_LOGGER_TRACE(logger, __VA_ARGS__)
#define RANGELUA_LOG_DEBUG(logger, ...)    SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define RANGELUA_LOG_INFO(logger, ...)     SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define RANGELUA_LOG_WARN(logger, ...)     SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define RANGELUA_LOG_ERROR(logger, ...)    SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)
#define RANGELUA_LOG_CRITICAL(logger, ...) SPDLOG_LOGGER_CRITICAL(logger, __VA_ARGS__)

// Module-specific logging macros
#define LEXER_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::lexer(), __VA_ARGS__)
#define LEXER_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::lexer(), __VA_ARGS__)
#define LEXER_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::lexer(), __VA_ARGS__)
#define LEXER_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::lexer(), __VA_ARGS__)
#define LEXER_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::lexer(), __VA_ARGS__)

#define PARSER_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::parser(), __VA_ARGS__)
#define PARSER_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::parser(), __VA_ARGS__)
#define PARSER_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::parser(), __VA_ARGS__)
#define PARSER_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::parser(), __VA_ARGS__)
#define PARSER_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::parser(), __VA_ARGS__)

#define CODEGEN_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::codegen(), __VA_ARGS__)
#define CODEGEN_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::codegen(), __VA_ARGS__)
#define CODEGEN_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::codegen(), __VA_ARGS__)
#define CODEGEN_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::codegen(), __VA_ARGS__)
#define CODEGEN_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::codegen(), __VA_ARGS__)

#define VM_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::vm(), __VA_ARGS__)
#define VM_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::vm(), __VA_ARGS__)
#define VM_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::vm(), __VA_ARGS__)
#define VM_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::vm(), __VA_ARGS__)
#define VM_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::vm(), __VA_ARGS__)

#define MEMORY_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::memory(), __VA_ARGS__)
#define MEMORY_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::memory(), __VA_ARGS__)
#define MEMORY_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::memory(), __VA_ARGS__)
#define MEMORY_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::memory(), __VA_ARGS__)
#define MEMORY_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::memory(), __VA_ARGS__)

#define OPTIMIZER_LOG_TRACE(...)                                                                   \
    RANGELUA_LOG_TRACE(rangelua::utils::loggers::optimizer(), __VA_ARGS__)
#define OPTIMIZER_LOG_DEBUG(...)                                                                   \
    RANGELUA_LOG_DEBUG(rangelua::utils::loggers::optimizer(), __VA_ARGS__)
#define OPTIMIZER_LOG_INFO(...)                                                                    \
    RANGELUA_LOG_INFO(rangelua::utils::loggers::optimizer(), __VA_ARGS__)
#define OPTIMIZER_LOG_WARN(...)                                                                    \
    RANGELUA_LOG_WARN(rangelua::utils::loggers::optimizer(), __VA_ARGS__)
#define OPTIMIZER_LOG_ERROR(...)                                                                   \
    RANGELUA_LOG_ERROR(rangelua::utils::loggers::optimizer(), __VA_ARGS__)

#define GC_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::gc(), __VA_ARGS__)
