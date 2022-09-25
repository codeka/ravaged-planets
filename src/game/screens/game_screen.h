#pragma once

#include <memory>
#include <game/screens/screen.h>

namespace fw {
namespace sg {
class Scenegraph;
}
}

namespace game {
class World;

// This is the options screen for specifying the options for the game we're about to play.
class GameScreenOptions: public ScreenOptions {
public:
  std::string map_name;
};

// The game screen is the main screen that is used when a game is actually in progress.
class GameScreen: public Screen {
private:
  World *world_;
  std::shared_ptr<GameScreenOptions> options_;

public:
  GameScreen();
  virtual ~GameScreen();

  virtual void set_options(std::shared_ptr<ScreenOptions> opt);

  virtual void show();
  virtual void hide();

  virtual void update();
  virtual void render(fw::sg::Scenegraph &Scenegraph);
};

}
