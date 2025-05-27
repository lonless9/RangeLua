#pragma once

/**
 * @file patterns.hpp
 * @brief Modern C++20 design patterns for RangeLua
 * @version 0.1.0
 */

#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "concepts.hpp"
#include "error.hpp"
#include "types.hpp"

namespace rangelua::patterns {

    /**
     * @brief Modern C++20 Visitor pattern with type safety
     */
    template <typename ReturnType = void>
    class Visitor {
    public:
        virtual ~Visitor() = default;

        template <typename NodeType>
        ReturnType visit(const NodeType& node) {
            if constexpr (std::is_void_v<ReturnType>) {
                visit_impl(node);
            } else {
                return visit_impl(node);
            }
        }

    protected:
        // Use function overloading instead of virtual templates
        virtual ReturnType visit_impl_dispatch(const void* node, std::type_info const& type_info) = 0;

        template <typename NodeType>
        ReturnType visit_impl(const NodeType& node) {
            return visit_impl_dispatch(&node, typeid(NodeType));
        }
    };

    /**
     * @brief Visitable interface with CRTP for type safety
     */
    template <typename Derived, typename VisitorType>
    class Visitable {
    public:
        template <typename V>
        auto accept(V&& visitor) -> decltype(visitor.visit(static_cast<const Derived&>(*this))) {
            return visitor.visit(static_cast<const Derived&>(*this));
        }

        template <typename V>
        auto accept(V&& visitor) const -> decltype(visitor.visit(static_cast<const Derived&>(*this))) {
            return visitor.visit(static_cast<const Derived&>(*this));
        }
    };

    /**
     * @brief Strategy pattern with modern C++20 features
     */
    template <typename ContextType, typename ReturnType = void>
    class Strategy {
    public:
        virtual ~Strategy() = default;

        virtual ReturnType execute(ContextType& context) = 0;
        virtual ReturnType execute(const ContextType& context) const = 0;

        [[nodiscard]] virtual StringView name() const noexcept = 0;
    };

    /**
     * @brief Strategy context with dependency injection
     */
    template <typename ContextType, typename ReturnType = void>
    class StrategyContext {
    public:
        using StrategyPtr = UniquePtr<Strategy<ContextType, ReturnType>>;

        explicit StrategyContext(StrategyPtr strategy) : strategy_(std::move(strategy)) {}

        ReturnType execute() {
            if (!strategy_) {
                if constexpr (std::is_void_v<ReturnType>) {
                    return;
                } else {
                    throw std::runtime_error("No strategy set");
                }
            }
            return strategy_->execute(context_);
        }

        ReturnType execute() const {
            if (!strategy_) {
                if constexpr (std::is_void_v<ReturnType>) {
                    return;
                } else {
                    throw std::runtime_error("No strategy set");
                }
            }
            return strategy_->execute(context_);
        }

        void set_strategy(StrategyPtr strategy) {
            strategy_ = std::move(strategy);
        }

        [[nodiscard]] const ContextType& context() const noexcept { return context_; }
        [[nodiscard]] ContextType& context() noexcept { return context_; }

    private:
        StrategyPtr strategy_;
        ContextType context_;
    };

    /**
     * @brief Observer pattern with type-safe events
     */
    template <typename EventType>
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void notify(const EventType& event) = 0;
    };

    /**
     * @brief Observable subject with automatic cleanup
     */
    template <typename EventType>
    class Observable {
    public:
        using ObserverPtr = WeakPtr<Observer<EventType>>;

        void add_observer(SharedPtr<Observer<EventType>> observer) {
            std::lock_guard<std::mutex> lock(mutex_);
            observers_.push_back(observer);
        }

        void remove_observer(SharedPtr<Observer<EventType>> observer) {
            std::lock_guard<std::mutex> lock(mutex_);
            observers_.erase(
                std::remove_if(observers_.begin(), observers_.end(),
                    [&observer](const ObserverPtr& weak_obs) {
                        return weak_obs.expired() || weak_obs.lock() == observer;
                    }),
                observers_.end());
        }

        void notify_observers(const EventType& event) {
            std::lock_guard<std::mutex> lock(mutex_);

            // Clean up expired observers and notify active ones
            auto it = observers_.begin();
            while (it != observers_.end()) {
                if (auto observer = it->lock()) {
                    observer->notify(event);
                    ++it;
                } else {
                    it = observers_.erase(it);
                }
            }
        }

        [[nodiscard]] Size observer_count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return observers_.size();
        }

    private:
        mutable std::mutex mutex_;
        std::vector<ObserverPtr> observers_;
    };

    /**
     * @brief Command pattern with undo support
     */
    class Command {
    public:
        virtual ~Command() = default;

        virtual void execute() = 0;
        virtual void undo() = 0;
        [[nodiscard]] virtual bool can_undo() const noexcept = 0;

        [[nodiscard]] virtual StringView description() const noexcept = 0;
    };

    /**
     * @brief Command invoker with history
     */
    class CommandInvoker {
    public:
        void execute_command(UniquePtr<Command> command) {
            if (command) {
                command->execute();
                if (command->can_undo()) {
                    history_.push_back(std::move(command));
                    // Limit history size
                    if (history_.size() > max_history_size_) {
                        history_.erase(history_.begin());
                    }
                }
            }
        }

        bool undo_last() {
            if (!history_.empty()) {
                auto& last_command = history_.back();
                last_command->undo();
                history_.pop_back();
                return true;
            }
            return false;
        }

        void clear_history() {
            history_.clear();
        }

        [[nodiscard]] Size history_size() const noexcept {
            return history_.size();
        }

        void set_max_history_size(Size size) noexcept {
            max_history_size_ = size;
            while (history_.size() > max_history_size_) {
                history_.erase(history_.begin());
            }
        }

    private:
        std::vector<UniquePtr<Command>> history_;
        Size max_history_size_ = 100;
    };

    /**
     * @brief Factory pattern with registration
     */
    template <typename BaseType, typename KeyType = String>
    class Factory {
    public:
        using CreateFunction = std::function<UniquePtr<BaseType>()>;

        template <typename DerivedType, typename... Args>
        void register_type(const KeyType& key, Args&&... args) {
            creators_[key] = [args...] {
                return std::make_unique<DerivedType>(args...);
            };
        }

        void register_creator(const KeyType& key, CreateFunction creator) {
            creators_[key] = std::move(creator);
        }

        [[nodiscard]] UniquePtr<BaseType> create(const KeyType& key) const {
            auto it = creators_.find(key);
            if (it != creators_.end()) {
                return it->second();
            }
            return nullptr;
        }

        [[nodiscard]] bool is_registered(const KeyType& key) const {
            return creators_.find(key) != creators_.end();
        }

        [[nodiscard]] std::vector<KeyType> registered_keys() const {
            std::vector<KeyType> keys;
            keys.reserve(creators_.size());
            for (const auto& [key, _] : creators_) {
                keys.push_back(key);
            }
            return keys;
        }

    private:
        std::unordered_map<KeyType, CreateFunction> creators_;
    };

    /**
     * @brief RAII resource manager with automatic cleanup
     */
    template <typename ResourceType, typename DeleterType = std::default_delete<ResourceType>>
    class ResourceManager {
    public:
        explicit ResourceManager(ResourceType* resource, DeleterType deleter = DeleterType{})
            : resource_(resource, deleter) {}

        ~ResourceManager() = default;

        // Non-copyable, movable
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        ResourceManager(ResourceManager&&) noexcept = default;
        ResourceManager& operator=(ResourceManager&&) noexcept = default;

        [[nodiscard]] ResourceType* get() const noexcept {
            return resource_.get();
        }

        [[nodiscard]] ResourceType& operator*() const {
            return *resource_;
        }

        [[nodiscard]] ResourceType* operator->() const noexcept {
            return resource_.get();
        }

        [[nodiscard]] bool is_valid() const noexcept {
            return resource_ != nullptr;
        }

        ResourceType* release() noexcept {
            return resource_.release();
        }

        void reset(ResourceType* new_resource = nullptr) {
            resource_.reset(new_resource);
        }

    private:
        std::unique_ptr<ResourceType, DeleterType> resource_;
    };

}  // namespace rangelua::patterns
