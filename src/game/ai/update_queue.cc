#include <functional>
#include <boost/format.hpp>

//#include <luabind/error.hpp>
//#include <luabind/object.hpp>

#include <framework/framework.h>
#include <framework/lua.h>
#include <framework/timer.h>
#include <framework/logging.h>
#include <framework/exception.h>

#include <game/ai/update_queue.h>

namespace game {

//-------------------------------------------------------------------------

QueueEntry::QueueEntry(float dequeue_time, std::function<void()> fn) :
    dequeue_time(dequeue_time), fn(fn) {
}

//-------------------------------------------------------------------------

bool QueueEntryCmp::operator()(QueueEntry const &lhs, QueueEntry const &rhs) {
  // we want the one with the LOWEST dequeue_time to go first, so our comparison
  // is actually > (the default priority_queue would be <)
  return (lhs.dequeue_time > rhs.dequeue_time);
}

//-------------------------------------------------------------------------

UpdateQueue::UpdateQueue() :
    now_(0) {
}

void UpdateQueue::push(float timeout, std::function<void()> fn) {
  queue_.push(QueueEntry(now_ + timeout, fn));
}

void UpdateQueue::update() {
  // update the _now time
  now_ = fw::Framework::get_instance()->get_timer()->get_total_time();

  for (;;) {
    // if there's nothing queued, we're done.
    if (queue_.size() == 0)
      return;

    // get the top entry, if it's time hasn't run out yet, we're done.
    QueueEntry qntry = queue_.top();
    if (qntry.dequeue_time >= now_)
      return;

    // otherwise, remove the top element and call it's callback.
    queue_.pop();

    if (!qntry.fn) {
      fw::debug << "ERR: update function callback cannot be called because it's empty." << std::endl;
    } else {
      try {
        qntry.fn();
//      } catch (luabind::error &e) {
//        luabind::object error_msg(luabind::from_stack(e.state(), -1));
 //       fw::debug << boost::format("An exception occured executing a Lua update function\n%1%\n%2%")
//            % e.what() % error_msg << std::endl;
      } catch (std::exception &e) {
        std::string msg = boost::diagnostic_information(e);
        fw::debug << "ERR: an exception occurred executing an update function" << std::endl;
        fw::debug << msg << std::endl;
      }
    }
  }
}

}
