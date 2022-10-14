
#include <framework/timer.h>

namespace fw {

const float _fps_update_interval_microseconds = 2.0f * 1000000.0f;

Timer::Timer() :
    num_frames_(0), total_time_seconds_(0), update_time_seconds_(0), fps_(0), stopped_(true) {
}

void Timer::start() {
  if (!stopped_)
    return;

  start_time_point_ = curr_time_point_ = Clock::now();
  stopped_ = false;
}

void Timer::stop() {
  update();
  stopped_ = true;
}

// this needs to be called each frame in order to keep the correct fps_ count
void Timer::update() {
  if (stopped_)
    return;

  auto now = Clock::now();

  // note: the counter for _total_time will wrap around eventually, but for all intents
  // and purposes, it doesn't matter.
  update_time_ = now - curr_time_point_;
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(update_time_).count();
  update_time_seconds_ = ((float) microseconds / 1000000.f)/* / 10.0f*/;

  microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_point_).count();
  total_time_seconds_ = (float) microseconds / 1000000.0f;

  curr_time_point_ = now;
}

/** Called on the render thread whenever we render a frame. We use this to update FPS. */
void Timer::render() {
  // update the fps counter every now and then
  num_frames_++;
  auto micros_since_fps_update =
    std::chrono::duration_cast<std::chrono::microseconds>(curr_time_point_ - last_fps_update_).count();
  if (micros_since_fps_update >= _fps_update_interval_microseconds) {
    double time = static_cast<double>(micros_since_fps_update) / 1000000.0;
    fps_ = static_cast<float>(static_cast<double>(num_frames_) / time);

    last_fps_update_ = curr_time_point_;
    num_frames_ = 0;
  }

  auto now = Clock::now();
  auto frame_time_micros = std::chrono::duration_cast<std::chrono::microseconds>(now - last_frame_time_point_).count();
  frame_time_seconds_ = ((float)frame_time_micros / 1000000.f);
  last_frame_time_point_ = now;
}

}
