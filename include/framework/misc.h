#pragma once

#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

//#include "vertex_formats.h"
#include <framework/vector.h>

namespace fw {

// performs a simple "distance between line and point" to get the minimum distance between
// the line defined by the given start point and direction and the given point.
float distance_between_line_and_point(vector const &start,
    vector const &direction, vector const &point);

// gets the distance between the given line segment and point. this is different to the
// distance_between_line_and_point in that it looks at an infinite line, where as this one only looks
// at the given line *segment*
float distance_between_line_segment_and_point(vector const &start,
    vector const &end, vector const &point);

// returns the angle, in radians, between a and b
float angle_between(vector const &a, vector const &b);

// given a plane (defined by a point and a normal), and a vector (defined by
// a position and direction), gets the point where the line intersects the plane
vector point_plane_intersect(vector const &plane_pt, vector const &plane_normal,
    vector const &p_start, vector const &p_dir);

// constrain the given value to the given max/min. If it's outside, we'll wrap back (not clamp)
template<typename T>
inline T constrain(T value, T max_value, T min_value = 0) {
  T tmp = value;
  T range = max_value - min_value;

  while (tmp < min_value) {
    tmp += range;
  }
  while (tmp >= max_value) {
    tmp -= range;
  }

  return tmp;
}

// constrain the given value to the given max/min. If it's outside, we'll clamp (not wrap)
template<typename T>
inline T clamp(T value, T max_value, T min_value = 0) {
  if (value < min_value)
    return min_value;
  if (value > max_value)
    return max_value;
  return value;
}

// split a string into an array, using the given list of delimiters
template<typename T>
inline std::vector<T> split(std::string s, std::string delim = " \r\n\t") {
  boost::tokenizer<> tok(s,
      boost::char_delimiters_separator<char>(false, "", delim.c_str()));

  std::vector<T> values;
  for (boost::tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it) {
    values.push_back(boost::lexical_cast<T>(*it));
  }
  return values;
}

// generates a random float between 0 and 1.
float random();

// Seeds our random number generator with a value based on current time. Call on app startup.
void random_initialize();

// these are used to calculate distances and directions in a world that wraps
fw::vector get_direction_to(fw::vector const &from, fw::vector const &to,
    float wrap_x, float wrap_z);
float calculate_distance(fw::vector const &from, fw::vector const &to,
    float wrap_x, float wrap_z);

// linearly interpolates between the two given values, given a value between 0
// (meaning "last") and 1 (meaning "next")
template<typename T>
inline T lerp(T const &last, T const &next, float t) {
  return (last + (next - last) * t);
}

// an "iless" functor, similar to "std::less" but case insensitive
template<typename T>
struct iless: public std::binary_function<T, T, bool> {
  inline bool operator ()(T const &lhs, T const &rhs) const {
    return boost::algorithm::ilexicographical_compare(lhs, rhs);
  }
};

// an "iequal_to" functor, similar to "std::equal_to" but case insensitive
template<typename T>
struct iequal_to: public std::binary_function<T, T, bool> {
  inline bool operator()(T const &lhs, T const &rhs) const {
    return boost::algorithm::iequals(lhs, rhs);
  }
};

// an "ihash" functor, similar to "std::hash" but case insensitive
template<typename T>
struct ihash: public std::unary_function<T, std::size_t> {
  inline std::size_t operator()(T const &x) const {
    std::size_t seed = 0;
    std::locale locale;

    for (typename T::const_iterator it = x.begin(); it != x.end(); ++it) {
      boost::hash_combine(seed, std::toupper(*it, locale));
    }

    return seed;
  }
};

}
