#pragma once

#include <map>

typedef struct SDL_Cursor SDL_Cursor;

namespace fw {

/** Class for setting the cursor, making it invisible and so on. */
class cursor {
private:
  std::map<int, std::string> _cursor_stack;
  std::map<std::string, SDL_Cursor *> _loaded_cursors;
  bool _cursor_visible;

  SDL_Cursor *load_cursor(std::string const &name);
  void set_cursor_for_real(std::string const &name);
  void update_cursor();
public:
  cursor();
  ~cursor();

  void initialize();
  void destroy();

  /** Sets the cursor with the given priority to the given value. The highest priority cursor is displayed. */
  void set_cursor(int priority, std::string const &name);

  /** Sets whether or not the cursor should be visible. */
  void set_visible(bool is_visible);

  bool is_visible();
};

}
