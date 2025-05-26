#pragma once

/**
 * @file logger.hpp
 * @brief Logging system with spdlog integration
 * @version 0.1.0
 */

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../core/config.hpp"
#include "../core/types.hpp"

namespace rangelua::utils {

    /**
     * @brief Logger wrapper around spdlog
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
         * @brief Add file sink
         */
        static void add_file_sink(const String& filename);

        /**
         * @brief Set log pattern
         */
        static void set_pattern(const String& pattern);

    private:
        static std::shared_ptr<spdlog::logger> default_logger_;
        static std::vector<spdlog::sink_ptr> sinks_;
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

#define GC_LOG_TRACE(...) RANGELUA_LOG_TRACE(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_DEBUG(...) RANGELUA_LOG_DEBUG(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_INFO(...)  RANGELUA_LOG_INFO(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_WARN(...)  RANGELUA_LOG_WARN(rangelua::utils::loggers::gc(), __VA_ARGS__)
#define GC_LOG_ERROR(...) RANGELUA_LOG_ERROR(rangelua::utils::loggers::gc(), __VA_ARGS__)
