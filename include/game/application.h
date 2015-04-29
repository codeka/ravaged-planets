#pragma once

#include <framework/framework.h>

namespace rp {
class world;
class screen_stack;

class application: public fw::base_app {
public:
  application();
  ~application();

  virtual bool initialize(fw::framework *frmwrf);
  virtual void destroy();
  virtual void update(float dt);
  virtual void render(fw::sg::scenegraph &sg);

  screen_stack *get_screen() {
    return _screen;
  }

private:
  fw::framework *_framework;
  screen_stack *_screen;
};

}
