
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>
#include <framework/colour.h>
#include <framework/input.h>
#include <framework/timer.h>
#include <framework/logging.h>
#include <framework/scenegraph.h>
#include <framework/gui/window.h>
#include <framework/misc.h>

#include <game/world/world.h>
#include <game/editor/editor_terrain.h>
#include <game/editor/windows/open_file.h>
#include <game/editor/windows/message_box.h>
#include <game/editor/tools/heightfield_tool.h>

namespace ed {
class open_file_window;
}

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
  virtual void update(fw::vector const &) {
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
  virtual void update(fw::vector const &cursor_loc);
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

void raise_lower_brush::update(fw::vector const &cursor_loc) {
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
  virtual void update(fw::vector const &cursor_loc);
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

void level_brush::update(fw::vector const &cursor_loc) {
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
class heightfield_tool_window {
private:
  ed::heightfield_tool *_tool;

  bool radius_slider_value_changed(fw::gui::widget *w);
  bool import_clicked(fw::gui::widget *w);
  bool brush_raise_lower_selected(fw::gui::widget *w);
  bool brush_level_selected(fw::gui::widget *w);
  void open_file_file_selected(ed::open_file_window *open_file);

public:
  heightfield_tool_window(ed::heightfield_tool *tool);
  ~heightfield_tool_window();

  void initialize();
  void show();
  void hide();
};

heightfield_tool_window::heightfield_tool_window(ed::heightfield_tool *tool) :
    _tool(tool) {
}

heightfield_tool_window::~heightfield_tool_window() {
}

void heightfield_tool_window::initialize() {
/*
  _radius_slider = get_child < CEGUI::Slider > ("ToolHeightfield/Radius");
  _radius_slider->setCurrentValue((float) _tool->get_radius() / ed::heightfield_tool::max_radius);
  subscribe(_radius_slider, CEGUI::Slider::EventValueChanged,
      CEGUI::SubscriberSlot(&heightfield_tool_window::radius_slider_value_changed, this));

  subscribe("ToolHeightfield/ImportFromFile", CEGUI::PushButton::EventClicked,
      CEGUI::SubscriberSlot(&heightfield_tool_window::import_clicked, this));
  subscribe("ToolHeightfield/BrushRaiseLower", CEGUI::RadioButton::EventSelectStateChanged,
      CEGUI::SubscriberSlot(&heightfield_tool_window::brush_raise_lower_selected, this));
  subscribe("ToolHeightfield/BrushLevel", CEGUI::RadioButton::EventSelectStateChanged,
      CEGUI::SubscriberSlot(&heightfield_tool_window::brush_level_selected, this));*/
}

void heightfield_tool_window::show() {
/*
  CEGUI::RadioButton *btn = get_child < CEGUI::RadioButton > ("ToolHeightfield/BrushRaiseLower");
  btn->setSelected(true);
  _tool->set_brush(new raise_lower_brush());*/
}

void heightfield_tool_window::hide() {
}

bool heightfield_tool_window::radius_slider_value_changed(fw::gui::widget *w) {
  //int radius = (int) (_radius_slider->getCurrentValue() * ed::heightfield_tool::max_radius);
  //_tool->set_radius(radius);
  return true;
}

bool heightfield_tool_window::import_clicked(fw::gui::widget *w) {
  //ed::open_file->set_file_selected_handler(boost::bind(&heightfield_tool_window::open_file_file_selected, this, _1));
 // ed::open_file->show();
  return true;
}

bool heightfield_tool_window::brush_raise_lower_selected(fw::gui::widget *w) {
  //CEGUI::RadioButton *btn = get_child < CEGUI::RadioButton > ("ToolHeightfield/BrushRaiseLower");
  //if (btn->isSelected()) {
  //  _tool->set_brush(new raise_lower_brush());
  //}
  return true;
}

bool heightfield_tool_window::brush_level_selected(fw::gui::widget *w) {
  ///CEGUI::RadioButton *btn = get_child < CEGUI::RadioButton > ("ToolHeightfield/BrushLevel");
  //if (btn->isSelected()) {
  //  _tool->set_brush(new level_brush());
  //}
  return true;
}

void heightfield_tool_window::open_file_file_selected(ed::open_file_window *open_file) {
  //fw::bitmap bm(open_file->get_full_path().c_str());
  //_tool->import_heightfield(bm);
}

//-----------------------------------------------------------------------------
namespace ed {
REGISTER_TOOL("heightfield", heightfield_tool);

float heightfield_tool::max_radius = 6;

heightfield_tool::heightfield_tool(game::world *wrld) :
    tool(wrld), _radius(4), _brush(nullptr) {
  _wnd = new heightfield_tool_window(this);
}

heightfield_tool::~heightfield_tool() {
  delete _wnd;
}

void heightfield_tool::activate() {
  tool::activate();

  fw::input *inp = fw::framework::get_instance()->get_input();
  _keybind_tokens.push_back(
      inp->bind_key("Left-Mouse", fw::input_binding(std::bind(&heightfield_tool::on_key, this, _1, _2))));
  _keybind_tokens.push_back(
      inp->bind_key("Right-Mouse", fw::input_binding(std::bind(&heightfield_tool::on_key, this, _1, _2))));

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

  fw::vector cursor_loc = _terrain->get_cursor_location();
  if (_brush != nullptr) {
    _brush->update(cursor_loc);
  }
}

void heightfield_tool::import_heightfield(fw::bitmap &bm) {
  int terrain_width = _terrain->get_width();
  int terrain_length = _terrain->get_length();

  // resize the bitmap so it's the same size as our terrain (use high quality == 2)
  bm.resize(terrain_width, terrain_length, 2);

  // get the pixel data from the bitmap
  std::vector<uint32_t> const &pixels = bm.get_pixels();

  // now, for each pixel, we set the terrain height!
  for (int x = 0; x < terrain_width; x++) {
    for (int z = 0; z < terrain_length; z++) {
      fw::colour col = fw::colour::from_argb(pixels[x + (z * terrain_width)]);
      float val = col.grayscale();

      _terrain->set_vertex_height(x, z, val * 30.0f);
    }
  }
}

void heightfield_tool::render(fw::sg::scenegraph &scenegraph) {
  draw_circle(scenegraph, _terrain, _terrain->get_cursor_location(), (float) _radius);
}
}
