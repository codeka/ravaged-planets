#pragma once

#include <memory>

#include <game/entities/entity.h>

namespace ent {
class PositionComponent;
class MoveableComponent;

// This is the base class for "projectile" components which allow an Entity to act like a projectile (e.g. ballistic,
// missile, bullet, etc)
class ProjectileComponent: public EntityComponent {
protected:
  std::weak_ptr<Entity> target_;
  PositionComponent *target_position_;
  MoveableComponent *our_moveable_;
  PositionComponent *our_position_;

public:
  static const int identifier = 600;

  ProjectileComponent();
  virtual ~ProjectileComponent();

  virtual void set_target(std::weak_ptr<Entity> target);
  std::weak_ptr<Entity> get_target() const {
    return target_;
  }

  virtual void initialize();
  virtual void update(float dt);

  // this is called when we detect we've hit our target (or something else got in the way) not all projectiles
  // will actually "explode" but that's a good enough analogy. If hit is valid, we'll assume that's the Entity we
  // actually hit and give them some extra damage
  virtual void explode(std::shared_ptr<Entity> hit);

  virtual int get_identifier() {
    return identifier;
  }
};

// This is a "seeking" projectile component, which "seeks" it target (for example, missiles)
class SeekingProjectileComponent: public ProjectileComponent {
private:
  // The seeker takes a short amount of time before it "locks" on the target.
  // Basically, we simply don't start "seeking" until this time runs out.
  float time_to_lock_;

public:
  SeekingProjectileComponent();
  virtual ~SeekingProjectileComponent();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void update(float dt);
};

// This is a "ballistic" projectile component, which travels in a arc to it's target
class BallisticProjectileComponent: public ProjectileComponent {
private:
  // this is the maximum height above the firing point that we'll travel.
  float max_height_;

public:
  BallisticProjectileComponent();
  virtual ~BallisticProjectileComponent();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void set_target(std::weak_ptr<Entity> target);
};

}
