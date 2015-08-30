
#include <framework/logging.h>
#include <framework/timer.h>

namespace fw {

const float _fps_update_interval_microseconds = 2.0f * 1000000.0f;

timer::timer() :
    _num_frames(0), _total_time_seconds(0), _frame_time_seconds(0), _fps(0), _stopped(true) {
}

void timer::start() {
  if (!_stopped)
    return;

  _start_time_point = _curr_time_point = chrono_clock::now();
  _stopped = false;
}

void timer::stop() {
  update();
  _stopped = true;
}

// this needs to be called each frame in order to keep the correct _fps count
void timer::update() {
  if (_stopped)
    return;

  auto now = chrono_clock::now();

  // note: the counter for _total_time will wrap around eventually, but for all intents
  // and purposes, it doesn't matter.
  _frame_time = now - _curr_time_point;
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(_frame_time).count();
  _frame_time_seconds = ((float) microseconds / 1000000.f)/* / 10.0f*/;

  microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - _start_time_point).count();
  _total_time_seconds = (float) microseconds / 1000000.0f;

  _curr_time_point = now;
}

/** Called on the render thread whenever we render a frame. We use this to update FPS. */
void timer::render() {
  // update the fps counter every now and then
  _num_frames++;
  auto micros_since_fps_update =
    std::chrono::duration_cast<std::chrono::microseconds>(_curr_time_point - _last_fps_update).count();
  if (micros_since_fps_update >= _fps_update_interval_microseconds) {
    double time = static_cast<double>(micros_since_fps_update) / 1000000.0;
    fw::debug << "_num_frames = " << _num_frames << " time = " << time << std::endl;
    _fps = static_cast<float>(static_cast<double>(_num_frames) / time);

    _last_fps_update = _curr_time_point;
    _num_frames = 0;
  }
}

}
