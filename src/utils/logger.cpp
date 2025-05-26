/**
 * @file logger.cpp
 * @brief Logger implementation
 * @version 0.1.0
 */

#include <rangelua/utils/logger.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace rangelua::utils {

    std::shared_ptr<spdlog::logger> Logger::default_logger_;
    std::vector<spdlog::sink_ptr> Logger::sinks_;
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

        auto logger = std::make_shared<spdlog::logger>(module_name, sinks_.begin(), sinks_.end());
        logger->set_level(default_logger_->level());
        return logger;
    }

    void Logger::set_level(LogLevel level) {
        if (default_logger_) {
            default_logger_->set_level(level);
            for (auto& sink : sinks_) {
                sink->set_level(level);
            }
        }
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
        }
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
