
#include <memory>

#include <boost/foreach.hpp>

#include <framework/scenegraph.h>
#include <framework/graphics.h>
#include <framework/framework.h>
#include <framework/camera.h>
//#include <framework/effect.h>
//#include <framework/shadows.h>
#include <framework/vertex_buffer.h>
#include <framework/index_buffer.h>
#include <framework/logging.h>
#include <framework/exception.h>
#include <framework/misc.h>
//#include <framework/gui/cegui.h>

// this is the effect file we'll use for rendering shadow map(s)
//static boost::shared_ptr<fw::effect> shadow_fx;

//static bool is_rendering_shadow = false;
//static boost::shared_ptr<fw::shadow_source> shadowsrc;

namespace fw {

namespace sg {

//-------------------------------------------------------------------------------------
light::light() :
    _cast_shadows(false) {
}

light::light(fw::vector const &pos, fw::vector const &dir, bool cast_shadows) :
    _pos(pos), _dir(dir), _cast_shadows(cast_shadows) {
}

light::light(light const &copy) :
    _pos(copy._pos), _dir(copy._dir), _cast_shadows(copy._cast_shadows) {
}

light::~light() {
}

//-------------------------------------------------------------------------------------
node::node() :
    _world(fw::identity()), _parent(0), _cast_shadows(true), _primitive_type(primitive_whatever) {
}

node::~node() {
}

void node::add_child(std::shared_ptr<node> child) {
  child->_parent = this;
  _children.push_back(child);
}

void node::remove_child(std::shared_ptr<node> child) {
  auto it = std::find(_children.begin(), _children.end(), child);
  if (it != _children.end())
    _children.erase(it);
}

// get the effect file to use. if we don't have one defined, look at our parent and keep looking up at our parents
// until we find one.
std::shared_ptr<fw::effect> node::get_effect() const {
/*  boost::shared_ptr<fw::effect> fx = _fx;
  if (!fx) {
    node *parent = _parent;
    while (!fx && parent != 0) {
      fx = parent->_fx;
      parent = parent->_parent;
    }
  }

  return fx;*/ return nullptr;
}

void node::render(scenegraph *sg) {
  // if we're not a shadow caster, and we're rendering shadows, don't
  // render the node this time around
//  if (is_rendering_shadow && !_cast_shadows)
//    return;

  if (_vb) {
    int num_primitives;
    if (!_ib) {
      num_primitives = _vb->get_num_vertices();
      if (_primitive_type == primitive_linestrip)
        num_primitives -= 1;
      else if (_primitive_type == primitive_linelist)
        num_primitives /= 2;
      else
        BOOST_THROW_EXCEPTION(fw::exception()
            << fw::message_error_info("given primitive_type is not yet supported."));
    } else {
      num_primitives = _ib->get_num_indices();
      if (_primitive_type == primitive_trianglestrip)
        num_primitives -= 2;
      else if (_primitive_type == primitive_trianglelist)
        num_primitives /= 3;
      else if (_primitive_type == primitive_linestrip)
        num_primitives -= 1;
      else if (_primitive_type == primitive_linelist)
        num_primitives /= 2;
      else
        BOOST_THROW_EXCEPTION(fw::exception()
            << fw::message_error_info("given primitive_type is not yet supported."));
    }

    std::shared_ptr<fw::effect> fx = _fx;
//				if (is_rendering_shadow)
//					fx = shadow_fx;

    if (!fx) {
      render_nofx(num_primitives, sg);
    } else {
      render_fx(num_primitives, fx);
    }
  }

  // render the children as well (todo: pass transformations)
  BOOST_FOREACH(std::shared_ptr<node> &child_node, _children) {
    child_node->render(sg);
  }
}

// this is called when we're rendering a gvien effect
void node::render_fx(int num_primitives, std::shared_ptr<fw::effect> fx) {
/*  boost::shared_ptr<fw::effect_parameters> parameters;
  if (_fx_params) {
    parameters = _fx_params;
  } else {
    parameters = fx->create_parameters();
  }

  // add the worldview and worldviewproj parameters as well as shadow parameters
  fw::camera *cam = fw::framework::get_instance()->get_camera();
  if (cam != 0) {
    fw::matrix worldview = cam->get_view_matrix();
    worldview = _world * worldview;
    fw::matrix worldviewproj = worldview * cam->get_projection_matrix();

    parameters->set_matrix("worldviewproj", worldviewproj);
    parameters->set_matrix("worldview", worldview);

//				if (!is_rendering_shadow && shadowsrc)
//				{
//					fw::matrix view_to_light = cam->get_view_matrix();
//					view_to_light = view_to_light.inverse();
//					view_to_light *= shadowsrc->get_camera().get_view_matrix();
//					view_to_light *= shadowsrc->get_camera().get_projection_matrix();
//
//					fx->set_matrix("view_to_light", view_to_light);
//					fx->set_texture("shadow_map", shadowsrc->get_shadowmap());
//				}
  }

  setup_effect(fx);
  fw::effect_pass *passes = fx->begin(parameters);
  while (passes && passes->valid()) {
    passes->begin_pass();
    _vb->render(num_primitives, _primitive_type, _ib.get());
    passes->end_pass();
  }
  fx->end(passes);*/
}

void node::render_nofx(int num_primitives, scenegraph *) {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(_world.data());

  _vb->render(num_primitives, _primitive_type, _ib.get());

  glPopMatrix();
}

// called to set any additional parameters on the given effect
void node::setup_effect(std::shared_ptr<fw::effect> fx) {
}

void node::populate_clone(std::shared_ptr<node> clone) {
  clone->_cast_shadows = _cast_shadows;
  clone->_primitive_type = _primitive_type;
  clone->_vb = _vb;
  clone->_ib = _ib;
  clone->_fx = _fx;
//  if (_fx_params)
//    clone->_fx_params = _fx_params->clone();
  clone->_parent = _parent;
  clone->_world = _world;

  // clone the children as well!
  BOOST_FOREACH(std::shared_ptr<node> child, _children) {
    clone->_children.push_back(child->clone());
  }
}

std::shared_ptr<node> node::clone() {
  std::shared_ptr<node> clone(new node());
  populate_clone(clone);
  return clone;
}

}

//-----------------------------------------------------------------------------------------
// renders the scene!
void render(sg::scenegraph &scenegraph, fw::texture *render_target /*= 0*/,
    fw::colour clear_colour /*= fw::colour(1, 0, 0, 0)*/) {
  graphics *g = fw::framework::get_instance()->get_graphics();

//		if (shadow_fx == 0)
//		{
//			shadow_fx = shared_ptr<fw::effect>(new effect());
//			shadow_fx->initialise("shadow.fx");
//		}

  if (render_target != 0)
    g->set_render_target(render_target);

  // set up the shadow sources that we'll need to render from first to
  // get the various shadows going...
//		typedef std::vector<boost::shared_ptr<shadow_source> > shadow_list;
//		shadow_list shadows;
//		for(sg::scenegraph::light_coll::const_iterator it = scenegraph.get_lights().begin(); it != scenegraph.get_lights().end(); ++it)
//		{
//			if ((*it)->get_cast_shadows())
//			{
//				shared_ptr<shadow_source> shdwsrc(new shadow_source());
//				shdwsrc->initialise();
//
//				light_camera &cam = shdwsrc->get_camera();
//				cam.set_location((*it)->get_position());
//				cam.set_direction((*it)->get_direction());
//
//				shadows.push_back(shdwsrc);
//			}
//		}

  // render the shadowmap(s) first
//		is_rendering_shadow = true;
//		BOOST_FOREACH(shadowsrc, shadows)
//		{
//			shadowsrc->begin_scene();
//			g->begin_scene();
//			for(sg::scenegraph::node_coll::const_iterator nit = scenegraph.get_nodes().begin(); nit != scenegraph.get_nodes().end(); ++nit)
//			{
//				(*nit)->render(&scenegraph);
//			}
//			g->end_scene();
//			shadowsrc->end_scene();
//		}
//		is_rendering_shadow = false;

  // now, render the main scene
  g->begin_scene(clear_colour);
  BOOST_FOREACH(std::shared_ptr<fw::sg::node> node, scenegraph.get_nodes()) {
    node->render(&scenegraph);
  }

  // make sure the shadowsrc is empty
//		if (shadowsrc)
//		{
//			shadowsrc.reset();
//		}

  if (render_target != 0) {
    g->end_scene();
    g->set_render_target(0);
  } else {
    // render the GUI now
    //framework::get_instance()->get_gui()->render();

    g->end_scene();
    g->present();
  }
}
}
