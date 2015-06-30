#pragma once

namespace ent {
class entity;
class entity_manager;
}

namespace game {
class terrain;

/**
 * This class "handles" the cursor. That is, it remembers which entity (if any) is currently under the cursor as well
 * what happens when you click the mouse button.
 */
class cursor_handler {
private:
  std::vector<int> _keybind_tokens;
  ent::entity_manager *_entities;
  terrain *_terrain;
  std::weak_ptr<ent::entity> _last_highlighted;
  std::weak_ptr<ent::entity> _entity_under_cursor;

  // this is called when the "select" button is pressed (left mouse button by default)
  void on_key_select(std::string keyname, bool is_down);

  // this is called when the "deselect" button is pressed (right mouse button by default)
  void on_key_deselect(std::string keyname, bool is_down);

public:
  cursor_handler();
  ~cursor_handler();

  void initialize();
  void update();
  void destroy();
};

}
