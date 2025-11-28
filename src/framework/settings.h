#pragma once

#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <framework/status.h>

namespace fw {

enum class SettingType {
  kUnknown,
  kString,
  kInt,
  kFloat,
  kBool,
};

// Similar to std::any, represents the value of a setting along with it's type info. Unlike
// std::any, we only support a limited set of types (string, int, float, bool) for settings.
struct SettingValue {
  explicit SettingValue(SettingType type) : type(type) {}

  SettingValue(const SettingValue& other) noexcept = default;
  SettingValue(SettingValue&& other) noexcept = default;
  SettingValue& operator=(const SettingValue& other) = default;

  SettingType type;
  std::string string_value;
  int int_value;
  float float_value;
  bool bool_value;

  template<typename T>
  inline static SettingType type_of() {
    if (typeid(T) == typeid(std::string)) {
      return SettingType::kString;
    } else if (typeid(T) == typeid(int)) {
      return SettingType::kInt;
    } else if (typeid(T) == typeid(float)) {
      return SettingType::kFloat;
    } else if (typeid(T) == typeid(bool)) {
      return SettingType::kBool;
    } else {
      return SettingType::kUnknown;
    }
  }

  template<typename T>
  inline T value() const;
  template<>
  inline std::string value<std::string>() const {
    if (type != SettingType::kString) {
      return "";
    }
    return string_value;
  }
  template<>
  inline bool value<bool>() const {
    if (type != SettingType::kBool) {
      return false;
    }
    return bool_value;
  }
  template<>
  inline int value<int>() const {
    if (type != SettingType::kInt) {
      return 0;
    }
    return int_value;
  }
  template<>
  inline float value<float>() const {
    if (type != SettingType::kFloat) {
      return 0.0f;
    }
    return float_value;
  }

  inline std::string value_str() const {
    switch (type) {
    case SettingType::kString:
      return string_value;
    case SettingType::kBool:
      return bool_value ? "true" : "false";
    case SettingType::kInt:
      return std::to_string(int_value);
    case SettingType::kFloat:
      return std::to_string(float_value);
    default:
      return "";
    }
  }

  template<typename T>
  static inline SettingValue of(T const &value);
  template<>
  static inline SettingValue of(std::string const &value) {
    SettingValue val(SettingType::kString);
    val.string_value = value;
    return val;
  }
  template<>
  static inline SettingValue of(bool const &value) {
    SettingValue val(SettingType::kBool);
    val.bool_value = value;
    return val;
  }
  template<>
  static inline SettingValue of(int const &value) {
    SettingValue val(SettingType::kInt);
    val.int_value = value;
    return val;
  }
  template<>
  static inline SettingValue of(float const &value) {
    SettingValue val(SettingType::kFloat);
    val.float_value = value;
    return val;
  }
};

// The complete definition of a single setting.
struct Setting {
  std::string name;
  std::string description;
  std::optional<SettingValue> default_value;
  SettingType type;
};

// A group of settings.
struct SettingGroup {
  std::string name;
  std::string description;

  std::vector<Setting> settings;
};

class SettingGroupBuilder {
public:
  inline SettingGroupBuilder(SettingGroup& group) : group_(group) {}

  template<typename T>
  inline SettingGroupBuilder& add_setting(std::string_view name,
      std::string_view description = "",
      std::optional<T> default_value = std::nullopt) {
    Setting setting;
    setting.name = name;
    setting.description = description;
    if (default_value.has_value()) {
      setting.default_value = SettingValue::of(*default_value);
    } else {
      setting.default_value = std::nullopt;
    }
    setting.type = SettingValue::type_of<T>();
    group_.settings.push_back(setting);
    return *this;
  }

private:
  SettingGroup& group_;
};

class SettingDefinition {
public:
  inline SettingGroupBuilder add_group(std::string_view name, std::string_view description = "") {
    SettingGroup group;
    group.name = name;
    group.description = description;
    groups_.push_back(group);
    return SettingGroupBuilder(groups_.back());
  }

  inline void merge(SettingDefinition const &other) {
    for (const SettingGroup &group : other.groups_) {
      groups_.push_back(group);
    }
  }

  inline std::deque<SettingGroup> const &groups() const {
    return groups_;
  }
private:
  std::deque<SettingGroup> groups_;
};

// This is the main "Settings" class, which you can use to access global Settings from the user's
// default.conf file.
class Settings {
public:
  Settings() = delete;
  ~Settings() = delete;

  // You must call this at program startup (*before* you call the Framework::initialize() method!)
  // it'll parse the command-line options, read the .conf file and so on.
  static fw::Status initialize(SettingDefinition const &additional_settings,
      int argc, char **argv, std::string_view options_file = "default.conf");

  // Gets the value (as a \c std::optional<SettingValue>) of a single variable. If the value is not
  // set, will return  nullopt.
  static std::optional<SettingValue> get(std::string_view name);

  // Gets the value (as a \c std::optional<T>) of a single variable. If the value is not set, or not
  // of type T, will return a default-constructed T.
  template<typename T>
  static inline T get(std::string_view name) {
    std::optional<SettingValue> const &val = get(name);
    if (!val.has_value()) {
      return T();
    }

    return val->value<T>();
  }

  // Get a value which indicates whether the given boolean option is specified.
  static inline bool is_set(std::string const &name) {
    std::optional<SettingValue> const &val = get(name);
    return val.has_value();
  }

  // Prints out the help message associated with our options.
  static void print_help();

  // Gets the full path to the executable.
  static std::filesystem::path get_executable_path();
};

}
