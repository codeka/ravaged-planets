#include <framework/stack_trace.h>

#include <string>
#include <vector>

#include <cpptrace/cpptrace.hpp>

namespace fw {

// Generates a stack trace of the current call stack, returning it as a vector of strings. By
// default, it skips the top 2 frames (that is, up to this function).
std::vector<std::string> GenerateStackTrace(int skip_frames /*= 2*/) {
  std::vector<std::string> entries;
  auto trace = cpptrace::generate_trace();
  for (auto entry : trace) {
    if (skip_frames-- > 0) {
      continue;
    }

    entries.push_back(entry.to_string());
  }

  return entries;
}

}