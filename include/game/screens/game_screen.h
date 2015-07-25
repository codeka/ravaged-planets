#pragma once

#include <memory>
#include <game/screens/screen.h>

namespace fw {
namespace sg {
class scenegraph;
}
}

namespace game {
class world;

/** This is the options screen for specifying the options for the game we're about to play. */
class game_screen_options: public screen_options {
public:
  std::string map_name;
};

/** The game screen is the main screen that is used when a game is actually in progress. */
class game_screen: public screen {
private:
  world *_world;
  std::shared_ptr<game_screen_options> _options;

public:
  game_screen();
  virtual ~game_screen();

  virtual void set_options(std::shared_ptr<screen_options> opt);

  virtual void show();
  virtual void hide();

  virtual void update();
  virtual void render(fw::sg::scenegraph &scenegraph);
};

}
