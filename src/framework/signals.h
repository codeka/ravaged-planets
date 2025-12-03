#pragma once

#include <functional>
#include <map>

namespace fw {

struct SignalConnection {
  int id = -1;
};

// A Signal is an implementation of the signals and slots pattern.
template <typename... Args>
class Signal {
public:
  // Connect to this signal, returns an ID that you can later use to disconnect from the slot.
  inline SignalConnection Connect(std::function<void(Args...)> const &slot) {
    slots_[current_id_] = slot;
    return {.id = current_id_++};
  }

  // Emits the signal, calling all connected slots with the given arguments.
  void Emit(Args... args) {
    for (auto const &slot : slots_) {
      slot.second(args...);
    }
  }

  // Disconnect the slot with the given ID (that was returned from Connect).
  void Disconnect(SignalConnection connection) {
    slots_.erase(connection.id);
  }

  // Disconnect all slots.
  void DisconnectAll() {
    slots_.clear();
  }

private:
  std::map<int, std::function<void(Args...)>> slots_;
  int current_id_ = 0;
};

}  // namespace fw
