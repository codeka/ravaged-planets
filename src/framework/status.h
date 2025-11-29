#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <absl/base/attributes.h>

namespace fw {

// A status represents the result of an operation, possibly an error. Similar to absl::Status,
// though we have a few conveniences that make it more suitable for our use cases. For example,
// we do not support error codes, only a boolean success/failure state. Also, we can add details
// to a status with the << operator.
class [[nodiscard]] Status {
public:
  Status();
  Status(bool ok, std::string_view message);

  // Returns true if the status represents success.
  inline bool ok() const {
    return ok_;
  }

  // Returns the message associated with this status.
  inline std::string const& message() const {
    return message_;
  }

  // Returns the stack trace captured when this error status was created. This will only be
  // populated for non-OK statuses.
  inline std::vector<std::string> const& stack_trace() const {
    return stack_trace_;
  }

  // Streams additional data to the message.
  template <typename T>
  inline Status& operator<<(const T& value) {
    std::ostringstream oss;
    oss << value;
    message_.append(oss.str());
    return *this;
  }

private:
  bool ok_;
  std::string message_;

  // This is a stack trace captured when the error status was created.
  std::vector<std::string> stack_trace_;
};

// Prints status message to stream.
inline std::ostream& operator<<(std::ostream& os, const Status& status) {
  if (status.ok()) {
    return os << "OK";
  }
  os << "Error: ";
  os << status.message();
  for (const auto& line : status.stack_trace()) {
    os << "\n" << line;
  }
  return os;
}

// Construc a return an OK status.
Status OkStatus();

// Construct an return an error status, with the given message.
Status ErrorStatus(std::string_view message);

// Represents either a value of type T or an error Status.
template <typename T>
class [[nodiscard]] StatusOr {
public:
  StatusOr(Status const &status) : status_(status) {}
  StatusOr(T const &value) : value_(value) {}
  StatusOr(T &&value) noexcept : value_(std::move(value)) {}

  inline Status const &status() const {
    return status_;
  }

  inline bool ok() const {
    return status_.ok();
  }

  // Access to the value via operator *.
  const T& operator*() const& {
    return value_.value();
  }
  T& operator*() & {
    return value_.value();
  }
  const T&& operator*() const&& {
    return value_.value();
  }
  T&& operator*() && {
    return value_.value();
  }

  // Access to the value via operator ->.
  const T* operator->() const {
    return std::addressof(**this);
  }
  T* operator->() {
    return std::addressof(**this);
  }

  T const &value() const {
    return value_.value();
  }
  T& value() {
    return value_.value();
  }

 private:
  Status status_;
  std::optional<T> value_;
};

// Helper macro to return if an expression yields an error Status.
#define RETURN_IF_ERROR(expr)                                             \
  do {                                                                    \
    auto _status = expr;                                                  \
    if (!_status.ok()) {                                                  \
      return _status;                                                     \
    }                                                                     \
  } while (false)

#define ASSIGN_OR_RETURN_CONCAT(left, right) ASSIGN_OR_RETURN_CONCAT_IMPL(left, right)

#define ASSIGN_OR_RETURN_CONCAT_IMPL(left, right) left##right

#define ASSIGN_OR_RETURN(lhs, expr) \
  ASSIGN_OR_RETURN_IMPL(ASSIGN_OR_RETURN_CONCAT(_status_or_, __COUNTER__), lhs, expr)

#define ASSIGN_OR_RETURN_IMPL(status_or_name, lhs, expr) \
  auto status_or_name = expr;                            \
  RETURN_IF_ERROR(status_or_name.status());              \
  lhs = std::move(status_or_name.value())

}
