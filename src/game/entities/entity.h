#pragma once

#include <memory>

#include <framework/lua.h>
//#include <luabind/object.hpp>

#include <game/entities/entity_attribute.h>
#include <game/entities/entity_debug.h>

namespace fw {
class Graphics;
class xml_element;
namespace sg {
class scenegraph;
}
}

namespace ent {
class entity_debug_view;
class entity;
class entity_manager;

/**
 * This is the identifier for an entity, it's actually made up of two components, the
 * identifier of the player who created it, and a unique number that the player assigns.
 * In this way, we avoid collisions when two players create an entity at the same time.
 */
typedef uint32_t entity_id;

/**
 * This is the base class for components of entities. It's just got a couple of methods
 * and stuff that let us figure out how the component fits in and so on.
 */

class entity_component {
protected:
  std::weak_ptr<entity> _entity;

public:
  entity_component();
  virtual ~entity_component();

  /**
   * This is called once the entity we're attached to has all of it's components defined and so
   * on (we can query for other components, etc)
   */
  virtual void initialize() {
  }

  /** This is called each frame in the "update" round. */
  virtual void update(float) {
  }

  /** This is called when it's time to render. generate a scenegraph node and add it to the scenegraph. */
  virtual void render(fw::sg::scenegraph &, fw::Matrix const &) {
  }

  /** Loads this component with data from the given component template (Lua object). */
  virtual void apply_template(luabind::object const &tmpl) {
  }

  /**
   * This is called by the entity_factory when we're added to an entity. Do not override this and
   * instead wait for initialize() to be called.
   */
  void set_entity(std::weak_ptr<entity> ent) {
    _entity = ent;
  }

  /**
   * Gets the unique identifier for this component (e.g. render_component::identifier for the render component)
   */
  virtual int get_identifier() = 0;

  /**
   * Return true from here if you want people to be able to call entity_manager::get_entities_by_component<YOU>();
   */
  virtual bool allow_get_by_component() {
    return false;
  }
};

/**
 * The entity class is a "container" class that represents all of the entities in the game (including
 * buildings, unit, trees, etc) it's basically a collection of components and the combination of
 * components is what makes it unique.
 */
class entity {
private:
  friend class entity_manager;
  entity(entity_manager *mgr, entity_id id);

  std::map<int, entity_component *> _components;
  std::map<std::string, entity_attribute> _attributes;
  std::weak_ptr<entity> _creator;
  entity_id id_;
  float _create_time;
  std::string _name;

  entity_debug_flags _debug_flags;
  entity_debug_view *_debug_view;
  entity_manager *mgr_;
public:
  ~entity();

  // adds a new component to this entity, and gets the component with the given identifier
  void add_component(entity_component *comp);
  entity_component *get_component(int identifier);

  // determines whether we contain a component of the given type
  bool contains_component(int identifier) const;

  // adds an attribute, or gets a pointer to the attribute with the given name
  void add_attribute(entity_attribute const &attr);
  entity_attribute *get_attribute(std::string const &name);

  // gets the entity_manager we were created by
  entity_manager *get_manager() const {
    return mgr_;
  }

  // gets a reference to the entity which created us (e.g. if we're a missile
  // or something, this is the entity which fired us).
  std::weak_ptr<entity> get_creator() const {
    return _creator;
  }

  // this is called after all the components have been added and so on.
  void initialize();

  // this is a templated version of get_component that uses the fact that the components all
  // have a static member called "identifier" which contains the identifier
  template<class T>
  inline T *get_component() {
    return dynamic_cast<T *>(get_component(T::identifier));
  }
  template<class T>
  inline T const *get_component() const {
    return dynamic_cast<T const *>(get_component(T::identifier));
  }

  template<class T>
  inline bool contains_component() const {
    return contains_component(T::identifier);
  }

  // these are called each frame to update and then render the entity
  void update(float dt);
  void render(fw::sg::scenegraph &scenegraph, fw::Matrix const &transform);

  // this is a helper that you can use to move an entity directly to somewhere on the map.
  void set_position(fw::Vector const &pos);

  void set_debug_flags(entity_debug_flags flags) {
    _debug_flags = flags;
  }
  entity_debug_flags get_debug_flags() const {
    return _debug_flags;
  }
  entity_debug_view *get_debug_view();

  // gets a string description of the "name" of this entity
  std::string const &get_name() const {
    return _name;
  }
  float get_age() const;

  // gets the identifier for this entity
  entity_id get_id() const {
    return id_;
  }
};

}
