
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>
#include <framework/color.h>
#include <framework/exception.h>
#include <framework/input.h>
#include <framework/timer.h>
#include <framework/logging.h>
#include <framework/scenegraph.h>
#include <framework/gui/window.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/slider.h>
#include <framework/gui/label.h>
#include <framework/misc.h>

#include <game/editor/editor_terrain.h>
#include <game/editor/editor_world.h>
#include <game/editor/windows/open_file.h>
#include <game/editor/windows/message_box.h>
#include <game/editor/tools/heightfield_tool.h>

using namespace fw::gui;
using namespace std::placeholders;

//-----------------------------------------------------------------------------
/** This is the base "brush" class which handles the actual modification of the terrain. */
class heightfield_brush {
protected:
  ed::editor_terrain *_terrain;
  ed::heightfield_tool *_tool;

public:
  heightfield_brush();
  virtual ~heightfield_brush();

  void initialize(ed::heightfield_tool *tool, ed::editor_terrain *terrain);

  virtual void on_key(std::string, bool) {
  }
  virtual void update(fw::Vector const &) {
  }
};

heightfield_brush::heightfield_brush() :
    _tool(nullptr), _terrain(nullptr) {
}

heightfield_brush::~heightfield_brush() {
}

void heightfield_brush::initialize(ed::heightfield_tool *tool, ed::editor_terrain *terrain) {
  _tool = tool;
  _terrain = terrain;
}

//-----------------------------------------------------------------------------
class raise_lower_brush: public heightfield_brush {
private:
  enum raise_direction {
    none, up, down
  };
  raise_direction _raise_direction;

public:
  raise_lower_brush();
  virtual ~raise_lower_brush();

  virtual void on_key(std::string keyname, bool is_down);
  virtual void update(fw::Vector const &cursor_loc);
};

raise_lower_brush::raise_lower_brush() :
    _raise_direction(none) {
}

raise_lower_brush::~raise_lower_brush() {
}

void raise_lower_brush::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    if (is_down)
      _raise_direction = up;
    else
      _raise_direction = none;
  } else if (keyname == "Right-Mouse") {
    if (is_down)
      _raise_direction = down;
    else
      _raise_direction = none;
  }
}

void raise_lower_brush::update(fw::Vector const &cursor_loc) {
  float dy = 0.0f;
  if (_raise_direction == up) {
    dy += 5.0f * fw::framework::get_instance()->get_timer()->get_frame_time();
  } else if (_raise_direction == down) {
    dy -= 5.0f * fw::framework::get_instance()->get_timer()->get_frame_time();
  }

  if (dy != 0.0f) {
    int cx = (int) cursor_loc[0];
    int cz = (int) cursor_loc[2];
    int sx = cx - _tool->get_radius();
    int sz = cz - _tool->get_radius();
    int ex = cx + _tool->get_radius();
    int ez = cz + _tool->get_radius();

    for (int z = sz; z < ez; z++) {
      for (int x = sx; x < ex; x++) {
        float height = _terrain->get_vertex_height(x, z);
        float distance = sqrt((float) ((x - cx) * (x - cx) + (z - cz) * (z - cz))) / _tool->get_radius();
        if (distance < 1.0f)
          _terrain->set_vertex_height(x, z, height + (dy * (1.0f - distance)));
      }
    }
  }
}

//-----------------------------------------------------------------------------
class level_brush: public heightfield_brush {
private:
  bool _start;
  bool _levelling;
  float _level_height;

public:
  level_brush();
  virtual ~level_brush();

  virtual void on_key(std::string keyname, bool is_down);
  virtual void update(fw::Vector const &cursor_loc);
};

level_brush::level_brush() :
    _start(false), _levelling(false), _level_height(0.0f) {
}

level_brush::~level_brush() {
}

void level_brush::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    if (is_down) {
      _start = true;
    } else {
      _start = false;
      _levelling = false;
    }
  }
}

void level_brush::update(fw::Vector const &cursor_loc) {
  if (_start) {
    // this means you've just clicked the left-mouse button, we have to start
    // levelling, which means we need to save the current terrain height
    _level_height = _terrain->get_height(cursor_loc[0], cursor_loc[2]);
    _levelling = true;
    _start = false;
  }

  if (_levelling) {
    int cx = (int) cursor_loc[0];
    int cz = (int) cursor_loc[2];
    int sx = cx - _tool->get_radius();
    int sz = cz - _tool->get_radius();
    int ex = cx + _tool->get_radius();
    int ez = cz + _tool->get_radius();

    for (int z = sz; z < ez; z++) {
      for (int x = sx; x < ex; x++) {
        float height = _terrain->get_vertex_height(x, z);
        float diff = (_level_height - height) * fw::framework::get_instance()->get_timer()->get_frame_time();
        float distance = sqrt((float) ((x - cx) * (x - cx) + (z - cz) * (z - cz))) / _tool->get_radius();
        if (distance < 1.0f) {
          _terrain->set_vertex_height(x, z, height + (diff * (1.0f - distance)));
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
enum IDS {
  RAISE_LOWER_BRUSH_ID,
  LEVEL_BRUSH_ID
};

class heightfield_tool_window {
private:
  Window *_wnd;
  ed::heightfield_tool *_tool;

  void on_radius_updated(int ParticleRotation);
  bool on_tool_clicked(fw::gui::Widget *w);
  bool on_import_clicked(fw::gui::Widget *w);
  void on_import_file_selected(ed::open_file_window *ofw);

public:
  heightfield_tool_window(ed::heightfield_tool *tool);
  ~heightfield_tool_window();

  void show();
  void hide();
};

heightfield_tool_window::heightfield_tool_window(ed::heightfield_tool *tool) :
    _tool(tool) {
  _wnd = Builder<Window>(px(10), px(30), px(100), px(130)) << Window::background("frame")
      << (Builder<Button>(px(8), px(8), px(36), px(36)) << Widget::id(RAISE_LOWER_BRUSH_ID)
          << Button::icon("editor_hightfield_raiselower")
          << Button::click(std::bind(&heightfield_tool_window::on_tool_clicked, this, _1)))
      << (Builder<Button>(px(56), px(8), px(36), px(36)) << Widget::id(LEVEL_BRUSH_ID)
          << Button::icon("editor_hightfield_level")
          << Button::click(std::bind(&heightfield_tool_window::on_tool_clicked, this, _1)))
      << (Builder<Label>(px(4), px(52), sum(pct(100), px(-8)), px(18)) << Label::text("Size:"))
      << (Builder<Slider>(px(4), px(74), sum(pct(100), px(-8)), px(18))
          << Slider::limits(20, 100) << Slider::ParticleRotation(40)
          << Slider::on_update(std::bind(&heightfield_tool_window::on_radius_updated, this, _1)))
      << (Builder<Button>(px(4), px(96), sum(pct(100), px(-8)), px(30)) << Button::text("Import")
          << Button::click(std::bind(&heightfield_tool_window::on_import_clicked, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

heightfield_tool_window::~heightfield_tool_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void heightfield_tool_window::show() {
  _wnd->set_visible(true);
}

void heightfield_tool_window::hide() {
  _wnd->set_visible(false);
}

void heightfield_tool_window::on_radius_updated(int ParticleRotation) {
  int radius = ParticleRotation / 10;
  _tool->set_radius(radius);
}

bool heightfield_tool_window::on_import_clicked(fw::gui::Widget *w) {
  ed::open_file->show(std::bind(&heightfield_tool_window::on_import_file_selected, this, _1));
  return true;
}

void heightfield_tool_window::on_import_file_selected(ed::open_file_window *ofw) {
  try {
    fw::Bitmap bmp(ofw->get_selected_file());
    _tool->import_heightfield(bmp);
  } catch (fw::Exception &e) {
    // error, probably not a bitmap?
  }
}

bool heightfield_tool_window::on_tool_clicked(fw::gui::Widget *w) {
  Button *raise_lower = _wnd->find<Button>(RAISE_LOWER_BRUSH_ID);
  Button *level = _wnd->find<Button>(LEVEL_BRUSH_ID);

  raise_lower->set_pressed(false);
  level->set_pressed(false);
  dynamic_cast<Button *>(w)->set_pressed(true);

  if (w == raise_lower) {
    _tool->set_brush(new raise_lower_brush());
  } else if (w == level) {
    _tool->set_brush(new level_brush());
  }

  return true;
}

//-----------------------------------------------------------------------------
namespace ed {
REGISTER_TOOL("heightfield", heightfield_tool);

float heightfield_tool::max_radius = 6;

heightfield_tool::heightfield_tool(editor_world *wrld) :
    tool(wrld), _radius(4), _brush(nullptr) {
  _wnd = new heightfield_tool_window(this);
}

heightfield_tool::~heightfield_tool() {
  delete _wnd;
}

void heightfield_tool::activate() {
  tool::activate();

  fw::Input *inp = fw::framework::get_instance()->get_input();
  _keybind_tokens.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&heightfield_tool::on_key, this, _1, _2))));
  _keybind_tokens.push_back(
      inp->bind_key("Right-Mouse", fw::InputBinding(std::bind(&heightfield_tool::on_key, this, _1, _2))));

  if (_brush != nullptr) {
    delete _brush;
  }
  set_brush(new raise_lower_brush());

  _wnd->show();
}

void heightfield_tool::deactivate() {
  tool::deactivate();

  _wnd->hide();
}

void heightfield_tool::set_brush(heightfield_brush *brush) {
  delete _brush;
  _brush = brush;
  _brush->initialize(this, _terrain);
}

// This is called when you press or release one of the keys we use for
// interacting with the terrain (left mouse button for "raise", right mouse button
// for "lower" etc)
void heightfield_tool::on_key(std::string keyname, bool is_down) {
  if (_brush != nullptr) {
    _brush->on_key(keyname, is_down);
  }
}

void heightfield_tool::update() {
  tool::update();

  fw::Vector cursor_loc = _terrain->get_cursor_location();
  if (_brush != nullptr) {
    _brush->update(cursor_loc);
  }
}

void heightfield_tool::import_heightfield(fw::Bitmap &bm) {
  int terrain_width = _terrain->get_width();
  int terrain_length = _terrain->get_length();

  // resize the bitmap so it's the same size as our terrain
  bm.resize(terrain_width, terrain_length);

  // get the pixel data from the bitmap
  std::vector<uint32_t> const &pixels = bm.get_pixels();

  // now, for each pixel, we set the terrain height!
  for (int x = 0; x < terrain_width; x++) {
    for (int z = 0; z < terrain_length; z++) {
      fw::Color col = fw::Color::from_argb(pixels[x + (z * terrain_width)]);
      float val = col.grayscale();

      _terrain->set_vertex_height(x, z, val * 30.0f);
    }
  }
}

void heightfield_tool::render(fw::sg::scenegraph &scenegraph) {
  draw_circle(scenegraph, _terrain, _terrain->get_cursor_location(), (float) _radius);
}
}
