#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace fw {

/**
 * A work queue is basically a thread-safe queue that, when you call dequeue, it'll block until a work-item
 * is available.
 */
template<typename T>
class work_queue {
private:
  std::queue<T> _q;
  std::mutex _mutex;
  std::condition_variable _condition;

public:
  /** Remove an item from the queue, wait until an item is available if the queue is currently empty. */
  inline T dequeue();

  /** Add an item to the queue. */
  inline void enqueue(T const &val);
};

template<typename T>
T work_queue<T>::dequeue() {
  std::unique_lock<std::mutex> lock(_mutex);
  while (_q.size() == 0) {
    _condition.wait(lock);
  }

  T val = _q.front();
  _q.pop();

  return val;
}

template<typename T>
void work_queue<T>::enqueue(T const &val) {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _q.push(val);
  }

  _condition.notify_one();
}

}
