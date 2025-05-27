/**
 * @file logger.cpp
 * @brief Logger implementation
 * @version 0.1.0
 */

#include <rangelua/utils/logger.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <cctype>
#include <ranges>

namespace rangelua::utils {

    std::shared_ptr<spdlog::logger> Logger::default_logger_;
    std::vector<spdlog::sink_ptr> Logger::sinks_;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Logger::module_loggers_;
    std::unordered_map<std::string, Logger::LogLevel> Logger::module_levels_;
    bool Logger::initialized_ = false;

    void Logger::initialize(const String& name, LogLevel level) {
        if (initialized_) {
            return;
        }

        // Create console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(level);
        sinks_.push_back(console_sink);

        // Create default logger
        default_logger_ = std::make_shared<spdlog::logger>(name, sinks_.begin(), sinks_.end());
        default_logger_->set_level(level);

        // Register as default
        spdlog::set_default_logger(default_logger_);

        initialized_ = true;
    }

    void Logger::shutdown() {
        if (!initialized_) {
            return;
        }

        spdlog::shutdown();
        default_logger_.reset();
        sinks_.clear();
        initialized_ = false;
    }

    std::shared_ptr<spdlog::logger> Logger::get_logger() {
        return default_logger_;
    }

    std::shared_ptr<spdlog::logger> Logger::create_logger(const String& module_name) {
        if (!initialized_) {
            initialize();
        }

        // Check if logger already exists
        auto it = module_loggers_.find(module_name);
        if (it != module_loggers_.end()) {
            return it->second;
        }

        auto logger = std::make_shared<spdlog::logger>(module_name, sinks_.begin(), sinks_.end());

        // Set module-specific level if configured, otherwise use default
        auto level_it = module_levels_.find(module_name);
        if (level_it != module_levels_.end()) {
            logger->set_level(level_it->second);
        } else {
            logger->set_level(default_logger_->level());
        }

        module_loggers_[module_name] = logger;
        return logger;
    }

    void Logger::set_level(LogLevel level) {
        if (default_logger_) {
            default_logger_->set_level(level);
            for (auto& sink : sinks_) {
                sink->set_level(level);
            }

            // Update all module loggers that don't have specific levels set
            for (auto& [module_name, logger] : module_loggers_) {
                if (module_levels_.find(module_name) == module_levels_.end()) {
                    logger->set_level(level);
                }
            }
        }
    }

    void Logger::set_module_level(const String& module_name, LogLevel level) {
        module_levels_[module_name] = level;

        // Update existing logger if it exists
        auto it = module_loggers_.find(module_name);
        if (it != module_loggers_.end()) {
            it->second->set_level(level);
        }
    }

    Logger::LogLevel Logger::get_module_level(const String& module_name) {
        auto it = module_levels_.find(module_name);
        if (it != module_levels_.end()) {
            return it->second;
        }
        return default_logger_ ? default_logger_->level() : LogLevel::info;
    }

    void Logger::add_file_sink(const String& filename) {
        if (!initialized_) {
            initialize();
        }

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
        file_sink->set_level(default_logger_->level());
        sinks_.push_back(file_sink);

        // Recreate logger with new sinks
        default_logger_ =
            std::make_shared<spdlog::logger>(default_logger_->name(), sinks_.begin(), sinks_.end());
        default_logger_->set_level(sinks_[0]->level());
        spdlog::set_default_logger(default_logger_);
    }

    void Logger::set_pattern(const String& pattern) {
        if (default_logger_) {
            default_logger_->set_pattern(pattern);
            // Apply pattern to all module loggers
            for (auto& [module_name, logger] : module_loggers_) {
                logger->set_pattern(pattern);
            }
        }
    }

    void Logger::configure_from_args(const std::vector<std::string>& log_configs) {
        for (const auto& config : log_configs) {
            if (config.find(':') != std::string::npos) {
                // Module-specific configuration: "module:level"
                auto pos = config.find(':');
                std::string module = config.substr(0, pos);
                std::string level_str = config.substr(pos + 1);

                auto level = string_to_log_level(level_str);
                set_module_level(module, level);
            } else {
                // Global level configuration
                auto level = string_to_log_level(config);
                set_level(level);
            }
        }
    }

    Logger::LogLevel Logger::string_to_log_level(const std::string& level) {
        std::string lower_level = level;
        std::ranges::transform(lower_level, lower_level.begin(), ::tolower);

        if (lower_level == "trace") return LogLevel::trace;
        if (lower_level == "debug") return LogLevel::debug;
        if (lower_level == "info") return LogLevel::info;
        if (lower_level == "warn" || lower_level == "warning") return LogLevel::warn;
        if (lower_level == "error" || lower_level == "err") return LogLevel::err;
        if (lower_level == "critical" || lower_level == "crit") return LogLevel::critical;
        if (lower_level == "off" || lower_level == "none") return LogLevel::off;

        return LogLevel::info; // Default fallback
    }

    std::string Logger::log_level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::trace: return "trace";
            case LogLevel::debug: return "debug";
            case LogLevel::info: return "info";
            case LogLevel::warn: return "warn";
            case LogLevel::err: return "error";
            case LogLevel::critical: return "critical";
            case LogLevel::off: return "off";
            default: return "info";
        }
    }

    std::vector<std::string> Logger::get_available_modules() {
        return {"lexer", "parser", "codegen", "optimizer", "vm", "memory", "gc"};
    }

    namespace loggers {

        std::shared_ptr<spdlog::logger> lexer() {
            static auto logger = Logger::create_logger("lexer");
            return logger;
        }

        std::shared_ptr<spdlog::logger> parser() {
            static auto logger = Logger::create_logger("parser");
            return logger;
        }

        std::shared_ptr<spdlog::logger> codegen() {
            static auto logger = Logger::create_logger("codegen");
            return logger;
        }

        std::shared_ptr<spdlog::logger> optimizer() {
            static auto logger = Logger::create_logger("optimizer");
            return logger;
        }

        std::shared_ptr<spdlog::logger> vm() {
            static auto logger = Logger::create_logger("vm");
            return logger;
        }

        std::shared_ptr<spdlog::logger> memory() {
            static auto logger = Logger::create_logger("memory");
            return logger;
        }

        std::shared_ptr<spdlog::logger> gc() {
            static auto logger = Logger::create_logger("gc");
            return logger;
        }

    }  // namespace loggers

}  // namespace rangelua::utils
