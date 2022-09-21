#pragma once

#include <framework/framework.h>

namespace game {
class world;
class screen_stack;

class application: public fw::BaseApp {
public:
  application();
  ~application();

  virtual bool initialize(fw::Framework *frmwrf);
  virtual void destroy();
  virtual void update(float dt);
  virtual void render(fw::sg::Scenegraph &sg);

  screen_stack *get_screen() {
    return _screen;
  }

private:
  fw::Framework *_framework;
  screen_stack *_screen;
};

}
