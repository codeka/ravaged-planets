#include <framework/status.h>

#include <framework/stack_trace.h>

namespace fw {

Status::Status()
  : Status(true, "") {}

Status::Status(bool ok, std::string_view message)
  : ok_(ok), message_(message) {
  if (!ok_) {
    // Skip 3 frames: this constructor, and the GenerateStateTrace function itself.
    stack_trace_ = GenerateStackTrace(/* skip_frames= */ 3);
  }
}

Status OkStatus() {
  return Status::Status();
}

Status ErrorStatus(std::string_view message) {
  return Status::Status(false, message);
}

}
