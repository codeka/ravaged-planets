#pragma once

#include <memory>

#include <framework/lua.h>

#include <game/entities/entity_attribute.h>
#include <game/entities/entity_debug.h>

namespace fw {
class Graphics;
class XmlElement;
namespace sg {
class Scenegraph;
}
}

namespace ent {
class EntityDebugView;
class Entity;
class EntityManager;

/**
 * This is the identifier for an Entity, it's actually made up of two components, the
 * identifier of the player who created it, and a unique number that the player assigns.
 * In this way, we avoid collisions when two players create an Entity at the same time.
 */
typedef uint32_t entity_id;

/**
 * This is the base class for components of entities. It's just got a couple of methods
 * and stuff that let us figure out how the component fits in and so on.
 */

class EntityComponent {
protected:
  std::weak_ptr<Entity> entity_;

public:
  EntityComponent();
  virtual ~EntityComponent();

  /**
   * This is called once the Entity we're attached to has all of it's components defined and so
   * on (we can query for other components, etc)
   */
  virtual void initialize() {
  }

  /** This is called each frame in the "update" round. */
  virtual void update(float) {
  }

  /** This is called when it's time to render. generate a Scenegraph Node and add it to the Scenegraph. */
  virtual void render(fw::sg::Scenegraph &, fw::Matrix const &) {
  }

  /** Loads this component with data from the given component template (Lua object). */
  virtual void apply_template(fw::lua::Value tmpl) {
  }

  /**
   * This is called by the entity_factory when we're added to an Entity. Do not override this and
   * instead wait for initialize() to be called.
   */
  void set_entity(std::weak_ptr<Entity> ent) {
    entity_ = ent;
  }

  /**
   * Gets the unique identifier for this component (e.g. render_component::identifier for the render component)
   */
  virtual int get_identifier() = 0;

  /**
   * Return true from here if you want people to be able to call EntityManager::get_entities_by_component<YOU>();
   */
  virtual bool allow_get_by_component() {
    return false;
  }
};

/**
 * The Entity class is a "container" class that represents all of the entities in the game (including
 * buildings, unit, trees, etc) it's basically a collection of components and the combination of
 * components is what makes it unique.
 */
class Entity {
private:
  friend class EntityManager;
  Entity(EntityManager *mgr, entity_id id);

  std::map<int, EntityComponent *> components_;
  std::map<std::string, EntityAttribute> attributes_;
  std::weak_ptr<Entity> creator_;
  std::vector<std::function<void()>> cleanup_functions_;
  entity_id id_;
  float create_time_;
  std::string name_;

  EntityDebugFlags debug_flags_;
  EntityDebugView *debug_view_;
  EntityManager *mgr_;
public:
  ~Entity();

  // adds a new component to this Entity, and gets the component with the given identifier
  void add_component(EntityComponent *comp);
  EntityComponent *get_component(int identifier);

  // determines whether we contain a component of the given type
  bool contains_component(int identifier) const;

  // adds an attribute, or gets a pointer to the attribute with the given name
  void add_attribute(EntityAttribute const &attr);
  EntityAttribute *get_attribute(std::string const &name);

  // gets the EntityManager we were created by
  EntityManager *get_manager() const {
    return mgr_;
  }

  // gets a reference to the Entity which created us (e.g. if we're a missile
  // or something, this is the Entity which fired us).
  std::weak_ptr<Entity> get_creator() const {
    return creator_;
  }

  // Adds a function that'll be called to clean up any extra memory or resources associated with this entity.
  void add_cleanup_function(std::function<void()> fn) {
    cleanup_functions_.push_back(fn);
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

  // these are called each frame to update and then render the Entity
  void update(float dt);
  void render(fw::sg::Scenegraph &scenegraph, fw::Matrix const &transform);

  // this is a helper that you can use to move an Entity directly to somewhere on the map.
  void set_position(fw::Vector const &pos);

  void set_debug_flags(EntityDebugFlags flags) {
    debug_flags_ = flags;
  }
  EntityDebugFlags get_debug_flags() const {
    return debug_flags_;
  }
  EntityDebugView *get_debug_view();

  // gets a string description of the "name" of this Entity
  std::string const &get_name() const {
    return name_;
  }
  float get_age() const;

  // gets the identifier for this Entity
  entity_id get_id() const {
    return id_;
  }
};

}
