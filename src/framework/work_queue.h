#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace fw {

// A work queue is basically a thread-safe queue that, when you call dequeue, it'll block until a work-item
// is available.
template<typename T>
class WorkQueue {
private:
  std::queue<T> q_;
  std::mutex mutex_;
  std::condition_variable condition_;

public:
  /** Remove an item from the queue, wait until an item is available if the queue is currently empty. */
  inline T dequeue();

  /** Add an item to the queue. */
  inline void enqueue(T const &val);
};

template<typename T>
T WorkQueue<T>::dequeue() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (q_.size() == 0) {
    condition_.wait(lock);
  }

  T val = q_.front();
  q_.pop();

  return val;
}

template<typename T>
void WorkQueue<T>::enqueue(T const &val) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    q_.push(val);
  }

  condition_.notify_one();
}

}
