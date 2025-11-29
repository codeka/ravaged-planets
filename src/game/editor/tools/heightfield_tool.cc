
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
class HeightfieldBrush {
protected:
  std::shared_ptr<ed::EditorTerrain> terrain_;
  ed::HeightfieldTool *tool_;

public:
  HeightfieldBrush();
  virtual ~HeightfieldBrush();

  void initialize(ed::HeightfieldTool *tool, std::shared_ptr<ed::EditorTerrain> terrain);

  virtual void on_key(std::string, bool) {
  }
  virtual void update(fw::Vector const &) {
  }
};

HeightfieldBrush::HeightfieldBrush() :
    tool_(nullptr), terrain_(nullptr) {
}

HeightfieldBrush::~HeightfieldBrush() {
}

void HeightfieldBrush::initialize(
    ed::HeightfieldTool *Tool,
    std::shared_ptr<ed::EditorTerrain> terrain) {
  tool_ = Tool;
  terrain_ = terrain;
}

//-----------------------------------------------------------------------------
class RaiseLowerBrush: public HeightfieldBrush {
private:
  enum RaiseDirection {
    none, up, down
  };
  RaiseDirection raise_direction_;

public:
  RaiseLowerBrush();
  virtual ~RaiseLowerBrush();

  virtual void on_key(std::string keyname, bool is_down);
  virtual void update(fw::Vector const &cursor_loc);
};

RaiseLowerBrush::RaiseLowerBrush() :
    raise_direction_(none) {
}

RaiseLowerBrush::~RaiseLowerBrush() {
}

void RaiseLowerBrush::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    if (is_down)
      raise_direction_ = up;
    else
      raise_direction_ = none;
  } else if (keyname == "Right-Mouse") {
    if (is_down)
      raise_direction_ = down;
    else
      raise_direction_ = none;
  }
}

void RaiseLowerBrush::update(fw::Vector const &cursor_loc) {
  float dy = 0.0f;
  if (raise_direction_ == up) {
    dy += 5.0f * fw::Framework::get_instance()->get_timer()->get_update_time();
  } else if (raise_direction_ == down) {
    dy -= 5.0f * fw::Framework::get_instance()->get_timer()->get_update_time();
  }

  if (dy != 0.0f) {
    int cx = (int) cursor_loc[0];
    int cz = (int) cursor_loc[2];
    int sx = cx - tool_->get_radius();
    int sz = cz - tool_->get_radius();
    int ex = cx + tool_->get_radius();
    int ez = cz + tool_->get_radius();

    for (int z = sz; z < ez; z++) {
      for (int x = sx; x < ex; x++) {
        float height = terrain_->get_vertex_height(x, z);
        float distance = sqrt((float) ((x - cx) * (x - cx) + (z - cz) * (z - cz))) / tool_->get_radius();
        if (distance < 1.0f)
          terrain_->set_vertex_height(x, z, height + (dy * (1.0f - distance)));
      }
    }
  }
}

//-----------------------------------------------------------------------------
class LevelBrush: public HeightfieldBrush {
private:
  bool start_;
  bool levelling_;
  float level_height_;

public:
  LevelBrush();
  virtual ~LevelBrush();

  virtual void on_key(std::string keyname, bool is_down);
  virtual void update(fw::Vector const &cursor_loc);
};

LevelBrush::LevelBrush() :
    start_(false), levelling_(false), level_height_(0.0f) {
}

LevelBrush::~LevelBrush() {
}

void LevelBrush::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    if (is_down) {
      start_ = true;
    } else {
      start_ = false;
      levelling_ = false;
    }
  }
}

void LevelBrush::update(fw::Vector const &cursor_loc) {
  if (start_) {
    // this means you've just clicked the left-mouse button, we have to start
    // levelling, which means we need to save the current terrain height
    level_height_ = terrain_->get_height(cursor_loc[0], cursor_loc[2]);
    levelling_ = true;
    start_ = false;
  }

  if (levelling_) {
    int cx = (int) cursor_loc[0];
    int cz = (int) cursor_loc[2];
    int sx = cx - tool_->get_radius();
    int sz = cz - tool_->get_radius();
    int ex = cx + tool_->get_radius();
    int ez = cz + tool_->get_radius();

    for (int z = sz; z < ez; z++) {
      for (int x = sx; x < ex; x++) {
        float height = terrain_->get_vertex_height(x, z);
        float diff = (level_height_ - height) * fw::Framework::get_instance()->get_timer()->get_update_time();
        float distance = sqrt((float) ((x - cx) * (x - cx) + (z - cz) * (z - cz))) / tool_->get_radius();
        if (distance < 1.0f) {
          terrain_->set_vertex_height(x, z, height + (diff * (1.0f - distance)));
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

class HeightfieldToolWindow {
private:
  Window *wnd_;
  ed::HeightfieldTool *tool_;

  void on_radius_updated(int ParticleRotation);
  bool on_tool_clicked(fw::gui::Widget *w);
  bool on_import_clicked(fw::gui::Widget *w);
  void on_import_file_selected(ed::OpenFileWindow *ofw);

public:
  HeightfieldToolWindow(ed::HeightfieldTool *Tool);
  ~HeightfieldToolWindow();

  void show();
  void hide();
};

HeightfieldToolWindow::HeightfieldToolWindow(ed::HeightfieldTool *Tool) :
    tool_(Tool) {
  wnd_ = Builder<Window>(px(10), px(30), px(100), px(130)) << Window::background("frame")
      << (Builder<Button>(px(8), px(8), px(36), px(36)) << Widget::id(RAISE_LOWER_BRUSH_ID)
          << Button::icon("editor_hightfield_raiselower")
          << Button::click(std::bind(&HeightfieldToolWindow::on_tool_clicked, this, _1)))
      << (Builder<Button>(px(56), px(8), px(36), px(36)) << Widget::id(LEVEL_BRUSH_ID)
          << Button::icon("editor_hightfield_level")
          << Button::click(std::bind(&HeightfieldToolWindow::on_tool_clicked, this, _1)))
      << (Builder<Label>(px(4), px(52), sum(pct(100), px(-8)), px(18)) << Label::text("Size:"))
      << (Builder<Slider>(px(4), px(74), sum(pct(100), px(-8)), px(18))
          << Slider::limits(20, 100) << Slider::ParticleRotation(40)
          << Slider::on_update(std::bind(&HeightfieldToolWindow::on_radius_updated, this, _1)))
      << (Builder<Button>(px(4), px(96), sum(pct(100), px(-8)), px(30)) << Button::text("Import")
          << Button::click(std::bind(&HeightfieldToolWindow::on_import_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

HeightfieldToolWindow::~HeightfieldToolWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void HeightfieldToolWindow::show() {
  wnd_->set_visible(true);
}

void HeightfieldToolWindow::hide() {
  wnd_->set_visible(false);
}

void HeightfieldToolWindow::on_radius_updated(int ParticleRotation) {
  int radius = ParticleRotation / 10;
  tool_->set_radius(radius);
}

bool HeightfieldToolWindow::on_import_clicked(fw::gui::Widget *w) {
  ed::open_file->show(std::bind(&HeightfieldToolWindow::on_import_file_selected, this, _1));
  return true;
}

void HeightfieldToolWindow::on_import_file_selected(ed::OpenFileWindow *ofw) {
  auto bmp = fw::load_bitmap(ofw->get_selected_file());
  if (!bmp.ok()) {
    // error, probably not an image?
    return;
  }
  tool_->import_heightfield(*bmp);
}

bool HeightfieldToolWindow::on_tool_clicked(fw::gui::Widget *w) {
  Button *raise_lower = wnd_->find<Button>(RAISE_LOWER_BRUSH_ID);
  Button *level = wnd_->find<Button>(LEVEL_BRUSH_ID);

  raise_lower->set_pressed(false);
  level->set_pressed(false);
  dynamic_cast<Button *>(w)->set_pressed(true);

  if (w == raise_lower) {
    tool_->set_brush(new RaiseLowerBrush());
  } else if (w == level) {
    tool_->set_brush(new LevelBrush());
  }

  return true;
}

//-----------------------------------------------------------------------------
namespace ed {
REGISTER_TOOL("heightfield", HeightfieldTool);

float HeightfieldTool::max_radius = 6;

HeightfieldTool::HeightfieldTool(EditorWorld *wrld) :
    Tool(wrld), radius_(4), brush_(nullptr) {
  wnd_ = new HeightfieldToolWindow(this);
}

HeightfieldTool::~HeightfieldTool() {
  delete wnd_;
}

void HeightfieldTool::activate() {
  Tool::activate();

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  keybind_tokens_.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&HeightfieldTool::on_key, this, _1, _2))));
  keybind_tokens_.push_back(
      inp->bind_key("Right-Mouse", fw::InputBinding(std::bind(&HeightfieldTool::on_key, this, _1, _2))));

  if (brush_ != nullptr) {
    delete brush_;
  }
  set_brush(new RaiseLowerBrush());

  indicator_ = std::make_shared<IndicatorNode>(terrain_);
  indicator_->set_radius(radius_);
  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [indicator = indicator_](fw::sg::Scenegraph& sg) {
      sg.add_node(indicator);
    });

  wnd_->show();
}

void HeightfieldTool::deactivate() {
  Tool::deactivate();

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [indicator = indicator_](fw::sg::Scenegraph& sg) {
      sg.remove_node(indicator);
    });
  indicator_.reset();

  wnd_->hide();
}

void HeightfieldTool::set_brush(HeightfieldBrush *brush) {
  delete brush_;
  brush_ = brush;
  brush_->initialize(this, terrain_);
}

// This is called when you press or release one of the keys we use for interacting with the terrain (left mouse button
// for "raise", right mouse button for "lower" etc)
void HeightfieldTool::on_key(std::string keyname, bool is_down) {
  if (brush_ != nullptr) {
    brush_->on_key(keyname, is_down);
  }
}

void HeightfieldTool::set_radius(int radius) {
  if (radius_ == radius) return;

  radius_ = radius;
  indicator_->set_radius(radius);
}

void HeightfieldTool::update() {
  Tool::update();

  fw::Vector cursor_loc = terrain_->get_cursor_location();
  if (brush_ != nullptr) {
    brush_->update(cursor_loc);
  }
  indicator_->set_center(cursor_loc);
}

void HeightfieldTool::import_heightfield(fw::Bitmap &bm) {
  int terrain_width = terrain_->get_width();
  int terrain_length = terrain_->get_length();

  // resize the bitmap so it's the same size as our terrain
  bm.resize(terrain_width, terrain_length);

  // get the pixel data from the bitmap
  std::vector<uint32_t> const &pixels = bm.get_pixels();

  // now, for each pixel, we set the terrain height!
  for (int x = 0; x < terrain_width; x++) {
    for (int z = 0; z < terrain_length; z++) {
      fw::Color col = fw::Color::from_argb(pixels[x + (z * terrain_width)]);
      float val = col.grayscale();

      terrain_->set_vertex_height(x, z, val * 30.0f);
    }
  }
}
/*
void HeightfieldTool::render(fw::sg::Scenegraph &scenegraph) {
  draw_circle(scenegraph, terrain_, terrain_->get_cursor_location(), (float) radius_);
}*/

}
