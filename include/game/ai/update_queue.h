#pragma once

#include <functional>
#include <deque>
#include <queue>

namespace game {

struct queue_entry {
  float dequeue_time;
  std::function<void()> fn;

  queue_entry(float dequeue_time, std::function<void()> fn);
};

/** A functor for comparing queue entries to ensure we always choose the one which is due out first. */
struct queue_entry_cmp {
  bool operator()(queue_entry const &lhs, queue_entry const &rhs);
};

/**
 * This class represents a queue of "update" functions. You schedule a function to run after a certain amount of time,
 * and we ensure that it runs at (approximately) that time.
 */
class update_queue {
private:
  std::priority_queue<queue_entry, std::deque<queue_entry>, queue_entry_cmp> _queue;
  float _now;

public:
  update_queue();

  // "push" the given callback to be called after given number of seconds have elapsed
  void push(float timeout, std::function<void()> fn);

  // call each frame; we'll call each of the callbacks whose time has expired
  void update();
};

}
