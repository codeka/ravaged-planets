#pragma once

#include <functional>
#include <deque>
#include <queue>

namespace game {

struct QueueEntry {
  float dequeue_time;
  std::function<void()> fn;

  QueueEntry(float dequeue_time, std::function<void()> fn);
};

// A functor for comparing queue entries to ensure we always choose the one which is due out first.
struct QueueEntryCmp {
  bool operator()(QueueEntry const &lhs, QueueEntry const &rhs);
};

// This class represents a queue of "update" functions. You schedule a function to run after a certain amount of time,
// and we ensure that it runs at (approximately) that time.
class UpdateQueue {
private:
  std::priority_queue<QueueEntry, std::deque<QueueEntry>, QueueEntryCmp> queue_;
  float now_;

public:
  UpdateQueue();

  // "push" the given callback to be called after given number of seconds have elapsed
  void push(float timeout, std::function<void()> fn);

  // call each frame; we'll call each of the callbacks whose time has expired
  void update();
};

}
