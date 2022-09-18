#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <boost/noncopyable.hpp>

#include <framework/vector.h>
#include <framework/work_queue.h>

namespace fw {
class path_find;
}

namespace game {
class terrain;

/**
 * Encapsulates a thread that all players will queue requests for path-finding to. We then execute one request
 * at a time and call the player back when the path is found.
 */
class pathing_thread: private boost::noncopyable {
public:
  typedef std::function<void(std::vector<fw::vector> const &)> callback_fn;

private:
  struct path_request_data {
    int flags;
    fw::vector start;
    fw::vector goal;
    callback_fn callback;
  };

  std::shared_ptr<fw::path_find> _pather;
  terrain *_terrain;
  std::thread _thread;
  fw::work_queue<path_request_data> _work_queue;

  void thread_proc();

public:
  pathing_thread();

  void start();
  void stop();

  void request_path(fw::vector const &start, fw::vector const &goal, callback_fn on_path_found);
};

}
