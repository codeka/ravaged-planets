#include <functional>
#include <boost/format.hpp>

#include <luabind/error.hpp>
#include <luabind/object.hpp>

#include <framework/framework.h>
#include <framework/lua.h>
#include <framework/timer.h>
#include <framework/logging.h>
#include <framework/exception.h>

#include <game/ai/update_queue.h>

namespace game {

//-------------------------------------------------------------------------

queue_entry::queue_entry(float dequeue_time, std::function<void()> fn) :
    dequeue_time(dequeue_time), fn(fn) {
}

//-------------------------------------------------------------------------

bool queue_entry_cmp::operator()(queue_entry const &lhs, queue_entry const &rhs) {
  // we want the one with the LOWEST dequeue_time to go first, so our comparison
  // is actually > (the default priority_queue would be <)
  return (lhs.dequeue_time > rhs.dequeue_time);
}

//-------------------------------------------------------------------------

update_queue::update_queue() :
    _now(0) {
}

void update_queue::push(float timeout, std::function<void()> fn) {
  _queue.push(queue_entry(_now + timeout, fn));
}

void update_queue::update() {
  // update the _now time
  _now = fw::framework::get_instance()->get_timer()->get_total_time();

  for (;;) {
    // if there's nothing queued, we're done.
    if (_queue.size() == 0)
      return;

    // get the top entry, if it's time hasn't run out yet, we're done.
    queue_entry qntry = _queue.top();
    if (qntry.dequeue_time >= _now)
      return;

    // otherwise, remove the top element and call it's callback.
    _queue.pop();

    if (!qntry.fn) {
      fw::debug << "ERR: update function callback cannot be called because it's empty." << std::endl;
    } else {
      try {
        qntry.fn();
      } catch (std::exception &e) {
        std::string msg = boost::diagnostic_information(e);
        fw::debug << "ERR: an exception occurred executing an update function" << std::endl;
        fw::debug << msg << std::endl;
      }
    }
  }
}

}
