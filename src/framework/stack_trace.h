#pragma once

#include <string>
#include <vector>

namespace fw {

// Generates a stack trace of the current call stack, returning it as a vector of strings. By
// default, it skips the top 2 frames (that is, up to this function).
std::vector<std::string> GenerateStackTrace(int skip_frames = 2);

}