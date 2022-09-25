#pragma once

namespace ent {
class Entity;
class EntityManager;
}

namespace game {
class Terrain;

// This class "handles" the cursor. That is, it remembers which Entity (if any) is currently under the cursor as well
// what happens when you click the mouse button.
class CursorHandler {
private:
  std::vector<int> keybind_tokens_;
  ent::EntityManager *entities_;
  Terrain *terrain_;
  std::weak_ptr<ent::Entity> last_highlighted_;
  std::weak_ptr<ent::Entity> entity_under_cursor_;

  // this is called when the "select" button is pressed (left mouse button by default)
  void on_key_select(std::string keyname, bool is_down);

  // this is called when the "deselect" button is pressed (right mouse button by default)
  void on_key_deselect(std::string keyname, bool is_down);

public:
  CursorHandler();
  ~CursorHandler();

  void initialize();
  void update();
  void destroy();
};

}
