/**
 * @file userdata.cpp
 * @brief Implementation of comprehensive Userdata API for RangeLua
 * @version 0.1.0
 */

#include <rangelua/api/userdata.hpp>
#include <rangelua/api/table.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

namespace rangelua::api {

    namespace {
        auto& logger() {
            static auto log = utils::Logger::create_logger("api.userdata");
            return log;
        }
    }  // namespace

    // Construction and conversion
    Userdata::Userdata() : userdata_(runtime::makeGCObject<runtime::Userdata>(nullptr, 0, "")) {
        logger()->debug("Created empty userdata");
    }

    Userdata::Userdata(runtime::Value value) {
        if (value.is_userdata()) {
            auto result = value.to_userdata();
            if (std::holds_alternative<runtime::Value::UserdataPtr>(result)) {
                userdata_ = std::get<runtime::Value::UserdataPtr>(result);
                logger()->debug("Created userdata wrapper from Value");
            } else {
                logger()->error("Failed to extract userdata from Value");
                throw std::invalid_argument("Failed to extract userdata from Value");
            }
        } else {
            logger()->error("Attempted to create Userdata from non-userdata Value");
            throw std::invalid_argument("Value is not userdata");
        }
    }

    Userdata::Userdata(runtime::GCPtr<runtime::Userdata> userdata) : userdata_(std::move(userdata)) {
        if (!userdata_) {
            logger()->error("Attempted to create Userdata from null GCPtr");
            throw std::invalid_argument("Userdata pointer is null");
        }
        logger()->debug("Created userdata wrapper from GCPtr");
    }

    Userdata::Userdata(Size size) : userdata_(runtime::makeGCObject<runtime::Userdata>(nullptr, size, "")) {
        logger()->debug("Created userdata with size: {}", size);
    }

    // Validation
    bool Userdata::is_valid() const noexcept {
        return userdata_.get() != nullptr;
    }

    bool Userdata::is_userdata() const noexcept {
        return is_valid();
    }

    // Data access
    void* Userdata::data() const {
        ensure_valid();
        return userdata_->data();
    }

    Size Userdata::size() const noexcept {
        if (!is_valid()) return 0;
        return userdata_->size();
    }

    bool Userdata::empty() const noexcept {
        return size() == 0;
    }

    // Data modification
    void Userdata::set_data(void* data, Size size) {
        ensure_valid();
        ensure_size(size);
        if (data && size > 0) {
            std::memcpy(userdata_->data(), data, size);
        }
    }

    // Type information
    String Userdata::type_name() const {
        ensure_valid();
        return userdata_->typeName();
    }

    void Userdata::set_type_name([[maybe_unused]] const String& name) {
        ensure_valid();
        // Note: runtime::Userdata doesn't support changing type name after creation
        // The type name is set during construction and is immutable
        logger()->warn("set_type_name called but runtime::Userdata type name is immutable");
    }

    // Metatable operations
    std::optional<Table> Userdata::get_metatable() const {
        ensure_valid();
        auto mt = userdata_->metatable();
        if (mt) {
            return Table(mt);
        }
        return std::nullopt;
    }

    void Userdata::set_metatable(const Table& metatable) {
        ensure_valid();
        userdata_->setMetatable(metatable.get_table());
    }

    void Userdata::set_metatable(std::nullptr_t) {
        ensure_valid();
        userdata_->setMetatable(runtime::GCPtr<runtime::Table>{});
    }

    bool Userdata::has_metatable() const noexcept {
        if (!is_valid()) return false;
        return userdata_->metatable().get() != nullptr;
    }

    // Finalizer management
    void Userdata::set_finalizer([[maybe_unused]] Finalizer finalizer) {
        ensure_valid();
        // Note: runtime::Userdata doesn't support custom finalizers
        // Finalizers are handled by the GC system automatically
        logger()->warn("set_finalizer called but runtime::Userdata doesn't support custom finalizers");
    }

    void Userdata::clear_finalizer() {
        ensure_valid();
        // Note: runtime::Userdata doesn't support custom finalizers
        logger()->warn("clear_finalizer called but runtime::Userdata doesn't support custom finalizers");
    }

    bool Userdata::has_finalizer() const noexcept {
        // Note: runtime::Userdata doesn't support custom finalizers
        return false;
    }

    // Conversion and access
    runtime::Value Userdata::to_value() const {
        ensure_valid();
        return runtime::Value(userdata_);
    }

    runtime::GCPtr<runtime::Userdata> Userdata::get_userdata() const {
        return userdata_;
    }

    const runtime::Userdata& Userdata::operator*() const {
        ensure_valid();
        return *userdata_;
    }

    const runtime::Userdata* Userdata::operator->() const {
        ensure_valid();
        return userdata_.get();
    }

    // Comparison
    bool Userdata::operator==(const Userdata& other) const {
        return userdata_ == other.userdata_;
    }

    bool Userdata::operator!=(const Userdata& other) const {
        return !(*this == other);
    }

    // Raw data access
    std::byte* Userdata::raw_data() const {
        ensure_valid();
        return static_cast<std::byte*>(userdata_->data());
    }

    const std::byte* Userdata::raw_data_const() const {
        ensure_valid();
        return static_cast<const std::byte*>(userdata_->data());
    }

    // Private methods
    void Userdata::ensure_valid() const {
        if (!is_valid()) {
            logger()->error("Operation on invalid userdata");
            throw std::runtime_error("Userdata is not valid");
        }
    }

    void Userdata::ensure_size(Size required_size) const {
        if (size() < required_size) {
            logger()->error("Userdata size {} is less than required size {}", size(), required_size);
            throw std::runtime_error("Userdata size is insufficient");
        }
    }

    // Template method implementations
    template<typename T>
    Userdata::Userdata(T&& data) {
        // Allocate memory for the object
        void* memory = std::malloc(sizeof(T));
        if (!memory) {
            throw std::bad_alloc();
        }

        // Construct the object in the allocated memory
        new (memory) T(std::forward<T>(data));

        // Create the userdata with the allocated memory
        userdata_ = runtime::makeGCObject<runtime::Userdata>(memory, sizeof(T), typeid(T).name());
        logger()->debug("Created userdata from object of type: {}", typeid(T).name());
    }

    template<typename T>
    T* Userdata::as() const {
        ensure_valid();
        ensure_type<T>();
        return static_cast<T*>(userdata_->data());
    }

    template<typename T>
    const T* Userdata::as_const() const {
        ensure_valid();
        ensure_type<T>();
        return static_cast<const T*>(userdata_->data());
    }

    template<typename T>
    T& Userdata::get() const {
        ensure_valid();
        ensure_type<T>();
        return *static_cast<T*>(userdata_->data());
    }

    template<typename T>
    const T& Userdata::get_const() const {
        ensure_valid();
        ensure_type<T>();
        return *static_cast<const T*>(userdata_->data());
    }

    template<typename T>
    void Userdata::set(T&& value) {
        ensure_valid();
        ensure_size(sizeof(T));
        new (userdata_->data()) T(std::forward<T>(value));
        set_type_name(typeid(T).name());
    }

    template<typename T>
    void Userdata::set_copy(const T& value) {
        ensure_valid();
        ensure_size(sizeof(T));
        new (userdata_->data()) T(value);
        set_type_name(typeid(T).name());
    }

    template<typename T>
    bool Userdata::is_type() const {
        if (!is_valid()) return false;
        return type_name() == typeid(T).name() && size() >= sizeof(T);
    }

    template<typename T>
    void Userdata::ensure_type() const {
        if (!is_type<T>()) {
            logger()->error("Type mismatch: expected {}, got {}", typeid(T).name(), type_name());
            throw std::runtime_error("Userdata type mismatch");
        }
    }

    // Factory functions
    namespace userdata_factory {

        Userdata create(Size size) {
            return Userdata(size);
        }

        template<typename T>
        Userdata from_copy(const T& object) {
            auto userdata = Userdata(sizeof(T));
            userdata.set_copy(object);
            return userdata;
        }

        template<typename T>
        Userdata from_move(T&& object) {
            auto userdata = Userdata(sizeof(T));
            userdata.set(std::forward<T>(object));
            return userdata;
        }

        template<typename T, typename... Args>
        Userdata emplace(Args&&... args) {
            auto userdata = Userdata(sizeof(T));
            new (userdata.data()) T(std::forward<Args>(args)...);
            userdata.set_type_name(typeid(T).name());
            return userdata;
        }

        template<typename T>
        Userdata from_shared_ptr(std::shared_ptr<T> ptr) {
            auto userdata = Userdata(sizeof(std::shared_ptr<T>));
            userdata.set(std::move(ptr));
            return userdata;
        }

        template<typename T>
        Userdata from_unique_ptr(std::unique_ptr<T> ptr) {
            auto userdata = Userdata(sizeof(std::unique_ptr<T>));
            userdata.set(std::move(ptr));
            return userdata;
        }

    }  // namespace userdata_factory

    // TypedUserdata implementation
    template<typename T>
    TypedUserdata<T>::TypedUserdata(Userdata userdata) : userdata_(std::move(userdata)) {
        if (!userdata_.is_type<T>()) {
            throw std::invalid_argument("Userdata does not contain the expected type");
        }
    }

    template<typename T>
    TypedUserdata<T>::TypedUserdata(T&& value) : userdata_(userdata_factory::from_move(std::forward<T>(value))) {
    }

    template<typename T>
    TypedUserdata<T>::TypedUserdata(const T& value) : userdata_(userdata_factory::from_copy(value)) {
    }

    template<typename T>
    T& TypedUserdata<T>::get() {
        return userdata_.template get<T>();
    }

    template<typename T>
    const T& TypedUserdata<T>::get() const {
        return userdata_.template get_const<T>();
    }

    template<typename T>
    T* TypedUserdata<T>::operator->() {
        return userdata_.template as<T>();
    }

    template<typename T>
    const T* TypedUserdata<T>::operator->() const {
        return userdata_.template as_const<T>();
    }

    template<typename T>
    T& TypedUserdata<T>::operator*() {
        return get();
    }

    template<typename T>
    const T& TypedUserdata<T>::operator*() const {
        return get();
    }

    template<typename T>
    Userdata TypedUserdata<T>::to_userdata() const {
        return userdata_;
    }

    template<typename T>
    runtime::Value TypedUserdata<T>::to_value() const {
        return userdata_.to_value();
    }

    template<typename T>
    bool TypedUserdata<T>::is_valid() const noexcept {
        return userdata_.is_valid() && userdata_.template is_type<T>();
    }

}  // namespace rangelua::api
