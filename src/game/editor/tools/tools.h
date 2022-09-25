#pragma once

namespace fw {
class Graphics;
namespace sg {
class Scenegraph;
}
}

namespace game {
class Terrain;
}

// Use this macro to register a tool with the tool_factory
#define REGISTER_TOOL(name, type) \
  Tool *create_ ## type (EditorWorld *wrld) { return new type(wrld); } \
  ToolFactoryRegistrar reg_create_ ## type(name, create_ ## type)

namespace ed {
class EditorScreen;
class EditorTerrain;
class editor_application;
class EditorWorld;

// this is the base class for the editor tools. one tool is active at
// a time, and interacts in some way with the environment. it can also
// put up various bits of UI for controlling it, etc.
class Tool {
protected:
  EditorWorld *world_;
  EditorTerrain *terrain_;
  EditorScreen *editor_;

  // for each key you bind in your tool's activate() method, you should add
  // the bind token to this list. We'll unbind it automatically when deactive()
  // is called.
  std::vector<int> keybind_tokens_;

public:
  Tool(EditorWorld *wrld);
  virtual ~Tool();

  EditorTerrain *get_terrain() const {
    return terrain_;
  }
  EditorWorld *get_world() const {
    return world_;
  }

  // this is called when this tool becomes, or stops being the active tool
  virtual void activate();
  virtual void deactivate();

  virtual void update();
  virtual void render(fw::sg::Scenegraph &Scenegraph);
};

typedef Tool *(*create_tool_fn)(EditorWorld *wrld);

// This class is used by the REGISTER_TOOL macro to register a tool
// at Application startup.
class ToolFactoryRegistrar {
public:
  ToolFactoryRegistrar(std::string const &name, create_tool_fn fn);
};

// This class contains all of the tool definitions that were registered via the
// REGISTER_TOOL macro and allows you to create instances of the tool at runtime
class ToolFactory {
public:
  static Tool *create_tool(std::string const &name, EditorWorld *world);
};

// draws a circle, centred at the given point, with the given radius. The circle will be
// in the x/z plane
void draw_circle(fw::sg::Scenegraph &Scenegraph, game::Terrain *terrain,
    fw::Vector const &center, float radius);

}
