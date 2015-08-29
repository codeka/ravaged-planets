#pragma once

#include <chrono>

namespace fw {

typedef std::chrono::high_resolution_clock chrono_clock;

// This class represents a timer which lets us time various events and so on.
class timer {
private:
  chrono_clock::time_point _start_time_point;
  chrono_clock::time_point _curr_time_point;
  float _total_time_seconds;
  float _frame_time_seconds;
  chrono_clock::duration _frame_time;
  bool _stopped;

  chrono_clock::time_point _last_fps_update;
  unsigned int _num_frames;
  float _fps;

public:
  timer();
  void start();
  void stop();
  void update();
  void render();

  inline bool is_stopped() const {
    return _stopped;
  }
  inline float get_fps() const {
    return _fps;
  }
  inline float get_total_time() const {
    return _total_time_seconds;
  }
  inline float get_frame_time() const {
    return _stopped ? 0.000001f : _frame_time_seconds;
  }

  chrono_clock::duration get_frame_duration() const {
    return _frame_time;
  }
};

}
