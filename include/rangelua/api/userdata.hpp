#pragma once

/**
 * @file userdata.hpp
 * @brief Comprehensive Userdata API for RangeLua
 * @version 0.1.0
 *
 * Provides a high-level C++ interface for Lua userdata with RAII semantics,
 * type safety, and integration with the runtime objects system.
 */

#include <memory>
#include <typeinfo>
#include <vector>

#include "../core/types.hpp"
#include "../runtime/objects.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

    // Forward declarations
    class Table;

    /**
     * @brief High-level Userdata wrapper with comprehensive API
     *
     * Provides a safe, convenient interface for working with Lua userdata.
     * Automatically manages the underlying runtime::Userdata object and provides
     * type-safe operations with proper error handling.
     */
    class Userdata {
    public:
        // Construction and conversion
        explicit Userdata();
        explicit Userdata(runtime::Value value);
        explicit Userdata(runtime::GCPtr<runtime::Userdata> userdata);
        explicit Userdata(Size size);

        template<typename T>
        explicit Userdata(T&& data);

        // Copy and move semantics
        Userdata(const Userdata& other) = default;
        Userdata& operator=(const Userdata& other) = default;
        Userdata(Userdata&& other) noexcept = default;
        Userdata& operator=(Userdata&& other) noexcept = default;

        ~Userdata() = default;

        // Validation
        [[nodiscard]] bool is_valid() const noexcept;
        [[nodiscard]] bool is_userdata() const noexcept;

        // Data access
        [[nodiscard]] void* data() const;
        [[nodiscard]] Size size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        // Type-safe data access
        template<typename T>
        [[nodiscard]] T* as() const;

        template<typename T>
        [[nodiscard]] const T* as_const() const;

        template<typename T>
        [[nodiscard]] T& get() const;

        template<typename T>
        [[nodiscard]] const T& get_const() const;

        // Data modification
        void set_data(void* data, Size size);

        template<typename T>
        void set(T&& value);

        template<typename T>
        void set_copy(const T& value);

        // Type information
        [[nodiscard]] String type_name() const;
        void set_type_name(const String& name);

        template<typename T>
        [[nodiscard]] bool is_type() const;

        // Metatable operations
        [[nodiscard]] std::optional<Table> get_metatable() const;
        void set_metatable(const Table& metatable);
        void set_metatable(std::nullptr_t);
        [[nodiscard]] bool has_metatable() const noexcept;

        // Finalizer management
        using Finalizer = std::function<void(void*)>;
        void set_finalizer(Finalizer finalizer);
        void clear_finalizer();
        [[nodiscard]] bool has_finalizer() const noexcept;

        // Conversion and access
        [[nodiscard]] runtime::Value to_value() const;
        [[nodiscard]] runtime::GCPtr<runtime::Userdata> get_userdata() const;
        [[nodiscard]] const runtime::Userdata& operator*() const;
        [[nodiscard]] const runtime::Userdata* operator->() const;

        // Comparison
        bool operator==(const Userdata& other) const;
        bool operator!=(const Userdata& other) const;

        // Raw data access (advanced)
        [[nodiscard]] std::byte* raw_data() const;
        [[nodiscard]] const std::byte* raw_data_const() const;

    private:
        runtime::GCPtr<runtime::Userdata> userdata_;

        void ensure_valid() const;
        void ensure_size(Size required_size) const;

        template<typename T>
        void ensure_type() const;
    };

    /**
     * @brief Userdata factory functions
     */
    namespace userdata_factory {

        /**
         * @brief Create empty userdata with specified size
         */
        Userdata create(Size size);

        /**
         * @brief Create userdata from C++ object (copy)
         */
        template<typename T>
        Userdata from_copy(const T& object);

        /**
         * @brief Create userdata from C++ object (move)
         */
        template<typename T>
        Userdata from_move(T&& object);

        /**
         * @brief Create userdata with in-place construction
         */
        template<typename T, typename... Args>
        Userdata emplace(Args&&... args);

        /**
         * @brief Create userdata from shared_ptr
         */
        template<typename T>
        Userdata from_shared_ptr(std::shared_ptr<T> ptr);

        /**
         * @brief Create userdata from unique_ptr
         */
        template<typename T>
        Userdata from_unique_ptr(std::unique_ptr<T> ptr);

    }  // namespace userdata_factory

    /**
     * @brief Type-safe userdata wrapper for specific C++ types
     */
    template<typename T>
    class TypedUserdata {
    public:
        explicit TypedUserdata(Userdata userdata);
        explicit TypedUserdata(T&& value);
        explicit TypedUserdata(const T& value);

        // Access to wrapped object
        [[nodiscard]] T& get();
        [[nodiscard]] const T& get() const;
        [[nodiscard]] T* operator->();
        [[nodiscard]] const T* operator->() const;
        [[nodiscard]] T& operator*();
        [[nodiscard]] const T& operator*() const;

        // Conversion
        [[nodiscard]] Userdata to_userdata() const;
        [[nodiscard]] runtime::Value to_value() const;

        // Validation
        [[nodiscard]] bool is_valid() const noexcept;

    private:
        Userdata userdata_;
    };

    /**
     * @brief Convenience type aliases for common userdata types
     */
    using StringUserdata = TypedUserdata<String>;
    using VectorUserdata = TypedUserdata<std::vector<runtime::Value>>;

}  // namespace rangelua::api
