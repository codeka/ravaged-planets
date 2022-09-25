#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <boost/noncopyable.hpp>

#include <framework/vector.h>
#include <framework/work_queue.h>

namespace fw {
class PathFfind;
}

namespace game {
class Terrain;

/**
 * Encapsulates a thread that all players will queue requests for path-finding to. We then execute one request
 * at a time and call the player back when the path is found.
 */
class pathing_thread: private boost::noncopyable {
public:
  typedef std::function<void(std::vector<fw::Vector> const &)> callback_fn;

private:
  struct path_request_data {
    int flags;
    fw::Vector start;
    fw::Vector goal;
    callback_fn callback;
  };

  std::shared_ptr<fw::PathFfind> _pather;
  Terrain *_terrain;
  std::thread thread_;
  fw::WorkQueue<path_request_data> _work_queue;

  void thread_proc();

public:
  pathing_thread();

  void start();
  void stop();

  void request_path(fw::Vector const &start, fw::Vector const &goal, callback_fn on_path_found);
};

}
