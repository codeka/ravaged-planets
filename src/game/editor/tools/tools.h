#pragma once

namespace fw {
class Graphics;
namespace sg {
class Scenegraph;
}
}

namespace game {
class terrain;
}

// Use this macro to register a tool with the tool_factory
#define REGISTER_TOOL(name, type) \
  tool *create_ ## type (editor_world *wrld) { return new type(wrld); } \
  tool_factory_registrar reg_create_ ## type(name, create_ ## type)

namespace ed {
class editor_screen;
class editor_terrain;
class editor_application;
class editor_world;

// this is the base class for the editor tools. one tool is active at
// a time, and interacts in some way with the environment. it can also
// put up various bits of UI for controlling it, etc.
class tool {
protected:
  editor_world *_world;
  editor_terrain *_terrain;
  editor_screen *_editor;

  // for each key you bind in your tool's activate() method, you should add
  // the bind token to this list. We'll unbind it automatically when deactive()
  // is called.
  std::vector<int> _keybind_tokens;

public:
  tool(editor_world *wrld);
  virtual ~tool();

  editor_terrain *get_terrain() const {
    return _terrain;
  }
  editor_world *get_world() const {
    return _world;
  }

  // this is called when this tool becomes, or stops being the active tool
  virtual void activate();
  virtual void deactivate();

  virtual void update();
  virtual void render(fw::sg::Scenegraph &Scenegraph);
};

typedef tool *(*create_tool_fn)(editor_world *wrld);

// This class is used by the REGISTER_TOOL macro to register a tool
// at application startup.
class tool_factory_registrar {
public:
  tool_factory_registrar(std::string const &name, create_tool_fn fn);
};

// This class contains all of the tool definitions that were registered via the
// REGISTER_TOOL macro and allows you to create instances of the tool at runtime
class tool_factory {
public:
  static tool *create_tool(std::string const &name, editor_world *world);
};

// draws a circle, centred at the given point, with the given radius. The circle will be
// in the x/z plane
void draw_circle(fw::sg::Scenegraph &Scenegraph, game::terrain *terrain,
    fw::Vector const &center, float radius);

}
