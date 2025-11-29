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

PathingThread::PathingThread() : terrain_(nullptr) {
}

void PathingThread::start() {
  // initialize the pather with the current world's map
  terrain_ = game::World::get_instance()->get_terrain();
  std::shared_ptr<fw::PathFind> pf(
      new fw::PathFind(
          terrain_->get_width(),
          terrain_->get_length(),
          terrain_->get_collision_data()));
  pather_ = pf;

  // start the thread that will simply wait for jobs to arrive and
  // then process them in order.
  thread_ = std::thread(std::bind(&PathingThread::thread_proc, this));
}

void PathingThread::stop() {
  // add an item to the queue to shutdown...
  PathRequestData request;
  request.flags = FLAG_STOP;
  work_queue_.enqueue(request);
}

void PathingThread::request_path(fw::Vector const &start, fw::Vector const &goal, callback_fn on_path_found) {
  PathRequestData request;
  request.flags = 0;
  request.start = start;
  request.goal = goal;
  request.callback = on_path_found;
  work_queue_.enqueue(request);
}

void PathingThread::thread_proc() {
  for (;;) {
    PathRequestData request = work_queue_.dequeue();
    if (request.flags == FLAG_STOP) {
      fw::debug << "pathing_thread::stop() has been called, thread_proc stopping." << std::endl;
      return;
    }

    std::vector<fw::Vector> path;
    pather_->find(path, request.start, request.goal);

    std::vector<fw::Vector> simplified;
    pather_->simplify_path(path, simplified);

    if (request.callback) {
      request.callback(simplified);
    }
  }
}
}
