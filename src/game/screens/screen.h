#pragma once

#include <memory>
#include <map>
#include <string>

namespace fw {
namespace sg {
class Scenegraph;
}
}

namespace game {

// This is the base class for the "Screen options" classes, which can be passed to a Screen when you set it as the
// active Screen. For example, the GameScreen uses this to pass things like map name, players, etc.
class ScreenOptions {
public:
  inline ScreenOptions() {
  }

  inline virtual ~ScreenOptions() {
  }
};

// This is the base class for the different screens in the game. A Screen is basically a "state" for the game, e.g.
// the title Screen the game Screen and so on.
class Screen {
public:
  Screen();
  virtual ~Screen();

  // Sets the ScreenOptions instance for this screen.
  virtual void set_options(std::shared_ptr<ScreenOptions> opt) {
  }

  // This is called when the screen is shown. you'll want to show your UI and anything else.
  virtual void show();

  // This is called when the Screen is hidden. For example, the title screen is hidden while a game is in progress (and
  // the game screen is being shown).
  virtual void hide();

  // Updates the "world", this is only called if we're the active screen.
  virtual void update();
};

// This class represents the "stack" of screens. the top-most Screen is the one that's currently being displayed.
class ScreenStack {
private:
  std::map<std::string, Screen *> screens_;
  std::string active_;

public:
  ScreenStack();
  ~ScreenStack();

  void set_active_screen(std::string const &name,
      std::shared_ptr<ScreenOptions> options = std::shared_ptr<ScreenOptions>());
  Screen *get_active_screen();
};

}
