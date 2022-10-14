#pragma once

#include <chrono>

namespace fw {

typedef std::chrono::high_resolution_clock Clock;

// This class represents a Timer which lets us time various events and so on.
class Timer {
private:
  Clock::time_point start_time_point_;
  Clock::time_point curr_time_point_;
  float total_time_seconds_;
  float update_time_seconds_;
  Clock::duration update_time_;
  bool stopped_;

  Clock::time_point last_fps_update_;
  unsigned int num_frames_;
  float frame_time_seconds_;
  Clock::time_point last_frame_time_point_;
  float fps_;

public:
  Timer();
  void start();
  void stop();
  void update();
  void render();

  inline bool is_stopped() const {
    return stopped_;
  }
  inline float get_fps() const {
    return fps_;
  }
  inline float get_total_time() const {
    return total_time_seconds_;
  }

  // Gets the time the last update iteration took.
  inline float get_update_time() const {
    return stopped_ ? 0.000001f : update_time_seconds_;
  }

  // Gets the time the last frame took to render.
  inline float get_frame_time() const {
    return frame_time_seconds_;
  }

  Clock::duration get_frame_duration() const {
    return update_time_;
  }
};

}
