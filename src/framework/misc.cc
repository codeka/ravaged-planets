
#include <random>
#include <math.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <cml/cml.h>

#include <framework/misc.h>
#include <framework/vector.h>

namespace fs = boost::filesystem;

// Our global random number generator.
static std::mt19937 rng;

namespace fw {

float distance_between_line_and_point(vector const &start,
    vector const &direction, vector const &point) {
  // see: http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
  vector end = start + direction;
  float u = (point[0] - start[0]) * (end[0] - start[0])
      + (point[1] - start[1]) * (end[1] - start[1])
      + (point[2] - start[2]) * (end[2] - start[2]);

  // this is the point on the line closest to 'point'
  vector point_on_line = start + (direction * u);

  // therefore, the distance is just the length of the (point - point_line_to) vector.
  vector vector_to_point = point - point_on_line;
  return vector_to_point.length();
}

// Gets the distance between the given line segment and point. This is different to
// distance_between_line_and_point, which looks at an infinite line, where as this one only looks
// at the given line *segment*.
float distance_between_line_segment_and_point(vector const &start,
    vector const &end, vector const &point) {
  // see: http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
  float u = (point[0] - start[0]) * (end[0] - start[0])
      + (point[1] - start[1]) * (end[1] - start[1])
      + (point[2] - start[2]) * (end[2] - start[2]);

  if (u < 0.0f)
    u = 0.0f;
  if (u > 1.0f)
    u = 1.0f;

  // this is the point on the line closest to 'point'
  vector point_on_line = start + ((end - start) * u);

  // therefore, the distance is just the length of the (point - point_line_to) vector.
  vector vector_to_point = point - point_on_line;
  return vector_to_point.length();
}

// Returns the angle, in radians, between a and b
float angle_between(vector const &a, vector const &b) {
  vector lhs = a;
  vector rhs = b;

  float cosangle = cml::dot(lhs.normalize(), rhs.normalize());
  return acos(cosangle);
}

fw::vector point_plane_intersect(vector const &plane_pt,
    vector const &plane_normal, vector const &p_start, vector const &p_dir) {
  // see: http://local.wasp.uwa.edu.au/~pbourke/geometry/planeline/
  vector end = p_start + p_dir;

  float numerator = cml::dot(plane_normal, plane_pt - p_start);
  float denominator = cml::dot(plane_normal, end - p_start);
  float u = numerator / denominator;
  return p_start + (p_dir * u);
}

float random() {
  return static_cast<float>(rng()) / static_cast<float>(rng.max());
}

void random_initialize() {
  std::random_device rd;
  rng.seed(rd());
}

fw::vector get_direction_to(fw::vector const &from, fw::vector const &to,
    float wrap_x, float wrap_z) {
  fw::vector dir = to - from;

  // if we're not wrapping, the direction is just the "simple" direction
  if (wrap_x == 0 && wrap_z == 0)
    return dir;

  // otherwise, we'll also try in the various other directions and return the shortest one
  for (int z = -1; z <= 1; z++) {
    for (int x = -1; x <= 1; x++) {
      fw::vector another_to(to[0] + (x * wrap_x), to[1], to[2] + (z * wrap_z));
      fw::vector another_dir = another_to - from;

      if (another_dir.length_squared() < dir.length_squared())
        dir = another_dir;
    }
  }

  return dir;
}

// Calculates the distance between 'from' and 'to', taking into consideration the fact that
// the world wraps at (wrap_x, wrap_z).
float calculate_distance(fw::vector const &from, fw::vector const &to,
    float wrap_x, float wrap_z) {
  return (get_direction_to(from, to, wrap_x, wrap_z).length());
}

}
