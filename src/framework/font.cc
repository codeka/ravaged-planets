
#include <mutex>

#include <boost/foreach.hpp>
#include <boost/locale.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <framework/bitmap.h>
#include <framework/exception.h>
#include <framework/font.h>
#include <framework/graphics.h>
#include <framework/lang.h>
#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/shader.h>
#include <framework/texture.h>

typedef std::basic_string<uint32_t> utf32string;

namespace fs = boost::filesystem;
namespace conv = boost::locale::conv;

#define FT_CHECK(fn) { \
  FT_Error err = fn; \
  if (err) { \
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(#fn)/*TODO << freetype_error_info(err)*/); \
  } \
}

namespace fw {

class glyph {
public:
  uint32_t ch;
  int glyph_index;
  int offset_x;
  int offset_y;
  float advance_x;
  float advance_y;
  int bitmap_left;
  int bitmap_top;
  int bitmap_width;
  int bitmap_height;
  float distance_from_baseline_to_top;
  float distance_from_baseline_to_bottom;

  glyph(uint32_t ch, int glyph_index, int offset_x, int offset_y, float advance_x,
      float advance_y, int bitmap_left, int bitmap_top, int bitmap_width, int bitmap_height,
      float distance_from_baseline_to_top, float distance_from_baseline_to_bottom);
  ~glyph();
};

glyph::glyph(uint32_t ch, int glyph_index, int offset_x, int offset_y, float advance_x,
    float advance_y, int bitmap_left, int bitmap_top, int bitmap_width, int bitmap_height,
    float distance_from_baseline_to_top, float distance_from_baseline_to_bottom) :
    ch(ch), glyph_index(glyph_index), offset_x(offset_x), offset_y(offset_y), advance_x(advance_x),
    advance_y(advance_y), bitmap_left(bitmap_left), bitmap_top(bitmap_top), bitmap_width(bitmap_width),
    bitmap_height(bitmap_height), distance_from_baseline_to_top(distance_from_baseline_to_top),
    distance_from_baseline_to_bottom(distance_from_baseline_to_bottom) {
}

glyph::~glyph() {
}

//-----------------------------------------------------------------------------

class string_cache_entry {
public:
  float time_since_use;
  std::shared_ptr<vertex_buffer> vb;
  std::shared_ptr<index_buffer> ib;
  std::shared_ptr<fw::shader> shader;
  std::shared_ptr<shader_parameters> shader_params;
  fw::Point size;
  float distance_to_top;
  float distance_to_bottom;

  string_cache_entry(std::shared_ptr<vertex_buffer> vb, std::shared_ptr<index_buffer> ib,
      std::shared_ptr<fw::shader> shader, std::shared_ptr<shader_parameters> shader_params,
      fw::Point size, float distance_to_top, float distance_to_bottom);
  ~string_cache_entry();
};

string_cache_entry::string_cache_entry(std::shared_ptr<vertex_buffer> vb,
    std::shared_ptr<index_buffer> ib, std::shared_ptr<fw::shader> shader,
    std::shared_ptr<shader_parameters> shader_params, fw::Point size,
    float distance_to_top, float distance_to_bottom) :
      vb(vb), ib(ib), shader(shader), shader_params(shader_params), time_since_use(0), size(size),
      distance_to_top(distance_to_top), distance_to_bottom(distance_to_bottom) {
}

string_cache_entry::~string_cache_entry() {
}

//-----------------------------------------------------------------------------

font_face::font_face(font_manager *manager, fs::path const &filename) :
    _manager(manager), _size(16) {
  FT_CHECK(FT_New_Face(_manager->_library, filename.string().c_str(), 0, &_face));
  fw::debug << "Loaded " << filename << ": " << _face->num_faces << " face(s) "
      << _face->num_glyphs << " glyph(s)" << std::endl;

  FT_CHECK(FT_Set_Pixel_Sizes(_face, 0, _size));

  // TODO: allow us to resize the bitmap?
  _bitmap = std::make_shared<fw::Bitmap>(256, 256);
  _texture = std::shared_ptr<fw::Texture>(new fw::Texture());
  _texture_dirty = true;
}

font_face::~font_face() {
  for(auto entry : _glyphs) {
    delete entry.second;
  }
}

void font_face::update(float dt) {
  // We actually want this to run on the render thread.
  fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    std::unique_lock<std::mutex> lock(_mutex);
    auto it = _string_cache.begin();
    while (it != _string_cache.end()) {
      // Note we update the time_since_use after adding dt. This ensures that if the thread time is really
      // long (e.g. if there's been some delay) we'll go through at least one update loop before destroying
      // the string.
      if (it->second->time_since_use > 1.0f) {
        _string_cache.erase(it++);
      } else {
        it->second->time_since_use += dt;
        ++it;
      }
    }
  });
}

void font_face::ensure_glyphs(std::string const &str) {
  ensure_glyphs(conv::utf_to_utf<uint32_t>(str));
}

void font_face::ensure_glyph(uint32_t ch) {
  if (_glyphs.find(ch) != _glyphs.end()) {
    // Already cached.
    return;
  }

  int glyph_index = FT_Get_Char_Index(_face, ch);

  // Load the glyph into the glyph slot and render it if it's not a bitmap
  FT_CHECK(FT_Load_Glyph(_face, glyph_index, FT_LOAD_DEFAULT));
  if (_face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
    // TODO: FT_RENDER_MODE_LCD?
    FT_CHECK(FT_Render_Glyph(_face->glyph, FT_RENDER_MODE_NORMAL));
  }

  // TODO: better bitmap packing than just a straight grid
  int glyphs_per_row = _bitmap->get_width() / _size;
  int offset_y = _glyphs.size() / glyphs_per_row;
  int offset_x = (_glyphs.size() - (offset_y * glyphs_per_row));
  offset_y *= _size;
  offset_x *= _size;

  for (int y = 0; y < _face->glyph->bitmap.rows; y++) {
    for (int x = 0; x < _face->glyph->bitmap.width; x++) {
      uint32_t rgba = 0x00ffffff | (
          _face->glyph->bitmap.buffer[y * _face->glyph->bitmap.width + x] << 24);
      _bitmap->set_pixel(offset_x + x, offset_y + y, rgba);
    }
  }

  std::string chstr = conv::utf_to_utf<char>(std::basic_string<uint32_t>({ch, 0}));
  _glyphs[ch] = new glyph(ch, glyph_index, offset_x, offset_y,
      _face->glyph->advance.x / 64.0f, _face->glyph->advance.y / 64.0f, _face->glyph->bitmap_left,
      _face->glyph->bitmap_top, _face->glyph->bitmap.width, _face->glyph->bitmap.rows,
      _face->glyph->metrics.horiBearingY / 64.0f,
      (_face->glyph->metrics.height - _face->glyph->metrics.horiBearingY) / 64.0f);
  _texture_dirty = true;
}

void font_face::ensure_glyphs(std::basic_string<uint32_t> const &str) {
  BOOST_FOREACH(uint32_t ch, str) {
    ensure_glyph(ch);
  }
}

fw::Point font_face::measure_string(std::string const &str) {
  return measure_string(conv::utf_to_utf<uint32_t>(str));
}

fw::Point font_face::measure_string(std::basic_string<uint32_t> const &str) {
  std::shared_ptr<string_cache_entry> data = get_or_create_cache_entry(str);
  return data->size;
}

fw::Point font_face::measure_substring(std::basic_string<uint32_t> const &str, int pos, int num_chars) {
  ensure_glyphs(str);

  fw::Point size(0, 0);
  for (int i = pos; i < pos + num_chars; i++) {
    fw::Point glyph_size = measure_glyph(str[i]);
    size[0] += glyph_size[0];
    if (size[1] < glyph_size[1]) {
      size[1] = glyph_size[1];
    }
  }

  return size;
}

fw::Point font_face::measure_glyph(uint32_t ch) {
  ensure_glyph(ch);
  glyph *g = _glyphs[ch];
  float y = g->distance_from_baseline_to_top + g->distance_from_baseline_to_bottom;
  return fw::Point(g->advance_x, y);
}

void font_face::draw_string(int x, int y, std::string const &str, draw_flags flags /*= 0*/,
    fw::Color color /*= fw::color::WHITE*/) {
  draw_string(x, y, conv::utf_to_utf<uint32_t>(str), flags, color);
}

void font_face::draw_string(int x, int y, std::basic_string<uint32_t> const &str, draw_flags flags, fw::Color color) {
  std::shared_ptr<string_cache_entry> data = get_or_create_cache_entry(str);

  if (_texture_dirty) {
    _texture->create(_bitmap);
  }

  if ((flags & align_centre) != 0) {
    x -= data->size[0] / 2;
  } else if (( flags & align_right) != 0) {
    x -= data->size[0];
  }
  if ((flags & align_top) != 0) {
    y += data->distance_to_top;
  } else if ((flags & align_middle) != 0) {
    y += (data->distance_to_top - data->distance_to_bottom) / 2;
  } else if ((flags & align_bottom) != 0) {
    y -= data->distance_to_bottom;
  }

  // TODO: recalculating this every time seems wasteful
  fw::graphics *g = fw::framework::get_instance()->get_graphics();
  fw::Matrix pos_transform;
  cml::matrix_orthographic_RH(pos_transform, 0.0f,
      static_cast<float>(g->get_width()), static_cast<float>(g->get_height()), 0.0f, 1.0f, -1.0f, cml::z_clip_neg_one);
  pos_transform = fw::translation(x, y, 0.0f) * pos_transform;

  data->shader_params->set_matrix("pos_transform", pos_transform);
  data->shader_params->set_matrix("uv_transform", fw::identity());
  data->shader_params->set_color("color", color);
  data->shader_params->set_texture("texsampler", _texture);

  data->vb->begin();
  data->ib->begin();
  data->shader->begin(data->shader_params);
  FW_CHECKED(glDrawElements(GL_TRIANGLES, data->ib->get_num_indices(), GL_UNSIGNED_SHORT, nullptr));
  data->shader->end();
  data->ib->end();
  data->vb->end();

  // Reset the timer so we keep this string cached.
  data->time_since_use = 0.0f;
}

std::shared_ptr<string_cache_entry> font_face::get_or_create_cache_entry(std::basic_string<uint32_t> const &str) {
  std::unique_lock<std::mutex> lock(_mutex);
  std::shared_ptr<string_cache_entry> data = _string_cache[str];
  if (data == nullptr) {
    data = create_cache_entry(str);
    _string_cache[str] = data;
  }

  return data;
}

std::shared_ptr<string_cache_entry> font_face::create_cache_entry(std::basic_string<uint32_t> const &str) {
  ensure_glyphs(str);

  std::vector<fw::vertex::xyz_uv> verts;
  verts.reserve(str.size() * 4);
  std::vector<uint16_t> indices;
  indices.reserve(str.size() * 6);
  float x = 0;
  float y = 0;
  float max_distance_to_top = 0.0f;
  float max_distance_to_bottom = 0.0f;
  BOOST_FOREACH(uint32_t ch, str) {
    glyph *g = _glyphs[ch];
    uint16_t index_offset = verts.size();
    verts.push_back(fw::vertex::xyz_uv(x + g->bitmap_left, y - g->bitmap_top, 0.0f,
        static_cast<float>(g->offset_x) / _bitmap->get_width(), static_cast<float>(g->offset_y) / _bitmap->get_height()));
    verts.push_back(fw::vertex::xyz_uv(x + g->bitmap_left, y - g->bitmap_top + g->bitmap_height, 0.0f,
        static_cast<float>(g->offset_x) / _bitmap->get_width(), static_cast<float>(g->offset_y + g->bitmap_height) / _bitmap->get_height()));
    verts.push_back(fw::vertex::xyz_uv(x + g->bitmap_left + g->bitmap_width, y - g->bitmap_top + g->bitmap_height, 0.0f,
        static_cast<float>(g->offset_x + g->bitmap_width) / _bitmap->get_width(), static_cast<float>(g->offset_y + g->bitmap_height) / _bitmap->get_height()));
    verts.push_back(fw::vertex::xyz_uv(x + g->bitmap_left + g->bitmap_width, y - g->bitmap_top, 0.0f,
        static_cast<float>(g->offset_x + g->bitmap_width) / _bitmap->get_width(), static_cast<float>(g->offset_y) / _bitmap->get_height()));

    indices.push_back(index_offset);
    indices.push_back(index_offset + 1);
    indices.push_back(index_offset + 2);
    indices.push_back(index_offset);
    indices.push_back(index_offset + 2);
    indices.push_back(index_offset + 3);

    x += g->advance_x;
    y += g->advance_y;
    if (max_distance_to_top < g->distance_from_baseline_to_top) {
      max_distance_to_top = g->distance_from_baseline_to_top;
    }
    if (max_distance_to_bottom < g->distance_from_baseline_to_bottom) {
      max_distance_to_bottom = g->distance_from_baseline_to_bottom;
    }
  }

  std::shared_ptr<fw::vertex_buffer> vb = fw::vertex_buffer::create<fw::vertex::xyz_uv>();
  vb->set_data(verts.size(), verts.data());

  std::shared_ptr<fw::index_buffer> ib = std::shared_ptr<fw::index_buffer>(new fw::index_buffer());
  ib->set_data(indices.size(), indices.data());

  std::shared_ptr<fw::shader> shader = fw::shader::create("gui.shader");
  std::shared_ptr<fw::shader_parameters> shader_params = shader->create_parameters();
  shader_params->set_program_name("font");

  return std::shared_ptr<string_cache_entry>(new string_cache_entry(vb, ib, shader, shader_params,
      fw::Point(x, max_distance_to_bottom + max_distance_to_top), max_distance_to_top, max_distance_to_bottom));
}

//-----------------------------------------------------------------------------

void font_manager::initialize() {
  FT_CHECK(FT_Init_FreeType(&_library));
}

void font_manager::update(float dt) {
  BOOST_FOREACH(auto it, _faces) {
    it.second->update(dt);
  }
}

std::shared_ptr<font_face> font_manager::get_face() {
  return get_face(fw::resolve("gui/" + fw::text("lang.font")));
}

/** Gets the \ref font_face for the font at the given path (assumed to be a .ttf file). */
std::shared_ptr<font_face> font_manager::get_face(fs::path const &filename) {
  // TODO: thread-safety?
  std::shared_ptr<font_face> face = _faces[filename.string()];
  if (!face) {
    face = std::shared_ptr<font_face>(new font_face(this, filename));
    _faces[filename.string()] = face;
  }
  return face;
}

}
