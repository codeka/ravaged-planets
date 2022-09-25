#include <functional>
#include <thread>

#include <framework/logging.h>
#include <framework/path_find.h>

#include <game/ai/pathing_thread.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

namespace game {

// "no" flags, the default
int FLAG_NONE = 0;

// if set, this means out stop() method has been called and the worker thread is to stop
int FLAG_STOP = 1;

pathing_thread::pathing_thread() : _terrain(nullptr) {
}

void pathing_thread::start() {
  // initialize the pather with the current world's map
  _terrain = game::World::get_instance()->get_terrain();
  std::shared_ptr<fw::PathFfind> pf(
      new fw::PathFfind(_terrain->get_width(), _terrain->get_length(), _terrain->get_collision_data()));
  _pather = pf;

  // start the thread that will simply wait for jobs to arrive and
  // then process them in order.
  thread_ = std::thread(std::bind(&pathing_thread::thread_proc, this));
}

void pathing_thread::stop() {
  // add an item to the queue to shutdown...
  path_request_data request;
  request.flags = FLAG_STOP;
  _work_queue.enqueue(request);
}

void pathing_thread::request_path(fw::Vector const &start, fw::Vector const &goal, callback_fn on_path_found) {
  path_request_data request;
  request.flags = 0;
  request.start = start;
  request.goal = goal;
  request.callback = on_path_found;
  _work_queue.enqueue(request);
}

void pathing_thread::thread_proc() {
  for (;;) {
    path_request_data request = _work_queue.dequeue();
    if (request.flags == FLAG_STOP) {
      fw::debug << "pathing_thread::stop() has been called, thread_proc stopping." << std::endl;
      return;
    }

    std::vector<fw::Vector> path;
    _pather->find(path, request.start, request.goal);

    std::vector<fw::Vector> simplified;
    _pather->simplify_path(path, simplified);

    if (request.callback) {
      request.callback(simplified);
    }
  }
}
}
