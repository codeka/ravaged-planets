#pragma once

#include <framework/framework.h>

namespace game {
class World;
class ScreenStack;

class Application: public fw::BaseApp {
public:
  Application();
  ~Application();

  bool initialize(fw::Framework *framework) override;
  void destroy() override;
  void update(float dt) override;
  
  ScreenStack *get_screen_stack() {
    return screen_stack_;
  }

private:
  fw::Framework *framework_;
  ScreenStack *screen_stack_;
};

}
