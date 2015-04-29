#pragma once

#include <string>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/vector.h>

struct effect_data;

namespace fw {

class graphics;
class texture;
class effect;
class colour;

// you can pass this to an effect to set a bunch of parameters all at once
class effect_parameters: private boost::noncopyable {
private:
  friend class effect;
  std::string _technique_name;

  std::map<std::string, boost::shared_ptr<texture> > _textures;
  std::map<std::string, matrix> _matrices;
  std::map<std::string, vector> _vectors;
  std::map<std::string, colour> _colours;
  std::map<std::string, float> _scalars;

  effect_parameters();
  void apply(effect *e) const;

public:
  ~effect_parameters();

  void set_technique_name(std::string const &name);
  void set_texture(std::string const &name, boost::shared_ptr<texture> const &t);
  void set_matrix(std::string const &name, matrix const &m);
  void set_vector(std::string const &name, vector const &v);
  void set_colour(std::string const &name, colour const &c);
  void set_scalar(std::string const &name, float f);

  boost::shared_ptr<effect_parameters> clone();
};

// this is returned by effect::begin() and is used to manage the state of a particular run of
// an effect
class effect_pass {
private:
 // CGpass _curr_pass;

public:
  effect_pass(/*CGpass pass*/);
  bool valid() const;

  void begin_pass();
  void end_pass();
};

// this class wraps CgFX files and allows us to automatically reload them, and so on.
class effect {
private:
  friend class effect_parameters;

 // CGtechnique _technique;
  boost::shared_ptr<effect_data> _data;

public:
  effect();
  ~effect();

  void initialise(boost::filesystem::path const &name);
  void set_technique(std::string const &name);

  // creates an effect_parameters that you'll pass to begin() in
  // order to set up the parameters for this rendering.
  boost::shared_ptr<effect_parameters> create_parameters();

  void set_texture(char const *name, texture const &t);
  void set_matrix(char const *name, matrix const &m);
  void set_vector(char const *name, vector const &v);
  void set_colour(char const *name, colour const &c);
  void set_scalar(char const *name, float f);

  effect_pass *begin(boost::shared_ptr<effect_parameters> parameters);
  void end(effect_pass *pass);
};

}
