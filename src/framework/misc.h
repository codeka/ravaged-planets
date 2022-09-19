#pragma once

#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

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

/**
 * Gets the name of the current user. Sometimes we may be able to return first name/last name, others just a username.
 */
std::string get_user_name();

// max and min are #define'd by windows.h ?_?
#undef max
#undef min

template<typename T>
inline T max(T a, T b) {
  return (a > b ? a : b);
}

template<typename T>
inline T min(T a, T b) {
  return (a < b ? a : b);
}

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
struct iless {
  bool operator()(T const &lhs, T const &rhs) const {
    return boost::algorithm::ilexicographical_compare(lhs, rhs);
  }
};

// an "iequal_to" functor, similar to "std::equal_to" but case insensitive
template<typename T>
struct iequal_to {
  bool operator()(T const &lhs, T const &rhs) const {
    return boost::algorithm::iequals(lhs, rhs);
  }
};

// an "ihash" functor, similar to "std::hash" but case insensitive
template<typename T>
struct ihash {
  inline std::size_t operator()(T const &x) const {
    std::size_t seed = 0;
    std::locale locale;

    for (typename T::const_iterator it = x.begin(); it != x.end(); ++it) {
      boost::hash_combine(seed, std::toupper(*it, locale));
    }

    return seed;
  }
};

/** Small utility class which represents a rectangle (x,y plus width,height). */
template<typename T>
class rectangle {
public:
  T left;
  T top;
  T width;
  T height;

  inline rectangle() : left(0), top(0), width(0), height(0) {
  }

  inline rectangle(T left, T top, T width, T height) : left(left), top(top), width(width), height(height) {
  }

  /** Returns the intersection of the two given rectangle. */
  static inline rectangle intersect(rectangle const &one, rectangle const &two) {
    const T x1 = one.left;
    const T x2 = one.left + one.width;
    const T x3 = two.left;
    const T x4 = two.left + two.width;
    const T y1 = one.top;
    const T y2 = one.top + one.height;
    const T y3 = two.top;
    const T y4 = two.top + two.height;

    const T x5 = max(x1, x3);
    const T x6 = min(x2, x4);
    const T y5 = max(y1, y3);
    const T y6 = min(y2, y4);

    return rectangle(x5, y5, x6-x5, y6-y5);
  }
};

}
