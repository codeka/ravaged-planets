#pragma once

#include <framework/framework.h>

namespace game {
class World;
class ScreenStack;

class Application: public fw::BaseApp {
public:
  Application();
  ~Application();

  virtual bool initialize(fw::Framework *frmwrf);
  virtual void destroy();
  virtual void update(float dt);
  virtual void render(fw::sg::Scenegraph &sg);

  ScreenStack *get_screen_stack() {
    return screen_stack_;
  }

private:
  fw::Framework *framework_;
  ScreenStack *screen_stack_;
};

}
