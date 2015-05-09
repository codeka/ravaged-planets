#pragma once

#include <game/screens/screen.h>

namespace rp {

/**
 * The title screen is the first screen we load up in the game. It contains the UI for starting a new game, loading a
 * save game, joining a multiplayer game and so on.
 */
class title_screen: public screen {
public:
  title_screen();
  virtual ~title_screen();

  virtual void show();
  virtual void hide();

  virtual void update();
};

}
