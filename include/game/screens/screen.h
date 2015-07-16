#pragma once

#include <memory>
#include <map>
#include <string>

namespace fw {
namespace sg {
class scenegraph;
}
}

namespace game {

/**
 * This is the base class for the "screen options" classes, which can be passed to a screen when you set it as the
 * active screen. For example, the game_screen uses this to pass things like map name, players, etc.
 */
class screen_options {
public:
  inline screen_options() {
  }

  inline virtual ~screen_options() {
  }
};

/**
 * This is the base class for the different screens in the game. A screen is basically a "state" for the game, e.g.
 * the title screen the game screen and so on.
 */
class screen {
public:
  screen();
  virtual ~screen();

  /** Sets the screen_options instance for this screen. */
  virtual void set_options(std::shared_ptr<screen_options> opt) {
  }

  /** This is called when the screen is shown. you'll want to show your UI and anything else. */
  virtual void show();

  /**
   * This is called when the screen is hidden. For example, the title screen is hidden while a game is in progress (and
   * the game screen is being shown).
   */
  virtual void hide();

  /** Updates the "world", this is only called if we're the active screen. */
  virtual void update();

  /** Renders this screen, this is only called if we're the active screen. */
  virtual void render(fw::sg::scenegraph &scenegraph);
};

/**
 * This class represents the "stack" of screens. the top-most screen is the one that's currently being displayed.
 */
class screen_stack {
private:
  std::map<std::string, screen *> _screens;
  std::string _active;

public:
  screen_stack();
  ~screen_stack();

  void set_active_screen(std::string const &name,
      std::shared_ptr<screen_options> options = std::shared_ptr<screen_options>());
  screen *get_active_screen();
};

}
