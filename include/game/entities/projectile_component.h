#pragma once

#include <memory>

#include <game/entities/entity.h>

namespace ent {
class position_component;
class moveable_component;

// This is the base class for "projectile" components which allow an
// entity to act like a projectile (e.g. ballistic, missile, bullet, etc)
class projectile_component: public entity_component {
protected:
  std::weak_ptr<entity> _target;
  position_component *_target_position;
  moveable_component *_our_moveable;
  position_component *_our_position;

public:
  static const int identifier = 600;

  projectile_component();
  virtual ~projectile_component();

  virtual void set_target(std::weak_ptr<entity> target);
  std::weak_ptr<entity> get_target() const {
    return _target;
  }

  virtual void initialize();
  virtual void update(float dt);

  // this is called when we detect we've hit our target (or something else got in the way) not all projectiles
  // will actually "explode" but that's a good enough analogy. If hit is valid, we'll assume that's the entity we
  // actually hit and give them some extra damage
  virtual void explode(std::shared_ptr<entity> hit);

  virtual int get_identifier() {
    return identifier;
  }
};

// This is a "seeking" projectile component, which "seeks" it target (for example, missiles)
class seeking_projectile_component: public projectile_component {
private:
  // The seeker takes a short amount of time before it "locks" on the target.
  // Basically, we simply don't start "seeking" until this time runs out.
  float _time_to_lock;

public:
  seeking_projectile_component();
  virtual ~seeking_projectile_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void update(float dt);
};

// This is a "ballistic" projectile component, which travels in a arc to it's target
class ballistic_projectile_component: public projectile_component {
private:
  // this is the maximum height above the firing point that we'll travel.
  float _max_height;

public:
  ballistic_projectile_component();
  virtual ~ballistic_projectile_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void set_target(std::weak_ptr<entity> target);
};

}
