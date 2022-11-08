
#include <random>
#include <math.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <framework/math.h>
#include <framework/misc.h>

namespace fs = boost::filesystem;

// Our global random number generator.
static std::mt19937 rng;

namespace fw {

float distance_between_line_and_point(Vector const &start,
    Vector const &direction, Vector const &point) {
  // see: Http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
  Vector end = start + direction;
  float u = (point[0] - start[0]) * (end[0] - start[0])
      + (point[1] - start[1]) * (end[1] - start[1])
      + (point[2] - start[2]) * (end[2] - start[2]);

  // this is the point on the line closest to 'point'
  Vector point_on_line = start + (direction * u);

  // therefore, the distance is just the length of the (point - point_line_to) vector.
  Vector vector_to_point = point - point_on_line;
  return vector_to_point.length();
}

// Gets the distance between the given line segment and point. This is different to
// distance_between_line_and_point, which looks at an infinite line, where as this one only looks
// at the given line *segment*.
float distance_between_line_segment_and_point(Vector const &start,
    Vector const &end, Vector const &point) {
  // see: Http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/
  float u = (point[0] - start[0]) * (end[0] - start[0])
      + (point[1] - start[1]) * (end[1] - start[1])
      + (point[2] - start[2]) * (end[2] - start[2]);

  if (u < 0.0f)
    u = 0.0f;
  if (u > 1.0f)
    u = 1.0f;

  // this is the point on the line closest to 'point'
  Vector point_on_line = start + ((end - start) * u);

  // therefore, the distance is just the length of the (point - point_line_to) vector.
  Vector vector_to_point = point - point_on_line;
  return vector_to_point.length();
}

// Returns the angle, in radians, between a and b
float angle_between(Vector const &a, Vector const &b) {
  Vector lhs = a;
  Vector rhs = b;

  float cosangle = dot(lhs.normalized(), rhs.normalized());
  return acos(cosangle);
}

fw::Vector point_plane_intersect(Vector const &plane_pt,
    Vector const &plane_normal, Vector const &p_start, Vector const &p_dir) {
  // see: Http://local.wasp.uwa.edu.au/~pbourke/geometry/planeline/
  Vector end = p_start + p_dir;

  float numerator = dot(plane_normal, plane_pt - p_start);
  float denominator = dot(plane_normal, end - p_start);
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

fw::Vector get_direction_to(fw::Vector const &from, fw::Vector const &to,
    float wrap_x, float wrap_z) {
  fw::Vector dir = to - from;

  // if we're not wrapping, the direction is just the "simple" direction
  if (wrap_x == 0 && wrap_z == 0)
    return dir;

  // otherwise, we'll also try in the various other directions and return the shortest one
  for (int z = -1; z <= 1; z++) {
    for (int x = -1; x <= 1; x++) {
      fw::Vector another_to(to[0] + (x * wrap_x), to[1], to[2] + (z * wrap_z));
      fw::Vector another_dir = another_to - from;

      if (another_dir.length() < dir.length())
        dir = another_dir;
    }
  }

  return dir;
}

// Calculates the distance between 'from' and 'to', taking into consideration the fact that
// the world wraps at (wrap_x, wrap_z).
float calculate_distance(fw::Vector const &from, fw::Vector const &to,
    float wrap_x, float wrap_z) {
  return (get_direction_to(from, to, wrap_x, wrap_z).length());
}

}
