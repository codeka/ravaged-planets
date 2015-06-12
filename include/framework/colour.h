#pragma once

#include <boost/format.hpp>

#include <framework/misc.h>

namespace fw {

// This class wraps simple access to ARGB data.
class colour {
public:
  float a, r, g, b;

  inline colour() :
      a(1.), r(0.), g(0.), b(0.) {
  }

  inline colour(float r, float g, float b) :
      a(1.0), r(r), g(g), b(b) {
  }

  inline colour(float a, float r, float g, float b) :
      a(a), r(r), g(g), b(b) {
  }

  inline colour(colour const &copy) :
      a(copy.a), r(copy.r), g(copy.g), b(copy.b) {
  }

  inline explicit colour(uint32_t rgba) {
    a = (rgba & 0x000000FF) / 255.0;
    b = ((rgba & 0x0000FF00) >> 8) / 255.0;
    g = ((rgba & 0x00FF0000) >> 16) / 255.0;
    r = ((rgba & 0xFF000000) >> 24) / 255.0;
  }

  inline colour &operator =(colour const &rhs) {
    a = rhs.a;
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    return *this;
  }

  inline colour operator +(colour const &rhs) const {
    return colour(a + rhs.a, r + rhs.r, g + rhs.g, b + rhs.b);
  }
  inline colour operator -(colour const &rhs) const {
    return colour(a - rhs.a, r - rhs.r, g - rhs.g, b - rhs.b);
  }
  inline colour operator *(float scalar) const {
    return colour(a * scalar, r * scalar, g * scalar, b * scalar);
  }
  inline colour operator /(float scalar) const {
    float inverse = 1.0 / scalar;
    return colour(a * inverse, r * inverse, g * inverse, b * inverse);
  }

  inline colour &operator +=(colour const &rhs) {
    a += rhs.a;
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    return *this;
  }
  inline colour &operator -=(colour const &rhs) {
    a -= rhs.a;
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    return *this;
  }
  inline colour &operator *=(float scalar) {
    a *= scalar;
    r *= scalar;
    g *= scalar;
    b *= scalar;
    return *this;
  }
  inline colour &operator /=(float scalar) {
    float inverse = 1.0 / scalar;
    a *= inverse;
    r *= inverse;
    g *= inverse;
    b *= inverse;
    return *this;
  }

  // clamps the colour to the valid range 0..1
  inline colour clamp() const {
    return colour(
        fw::clamp(a, 1.0f, 0.0f), fw::clamp(r, 1.0f, 0.0f), fw::clamp(g, 1.0f, 0.0f), fw::clamp(b, 1.0f, 0.0f));
  }

  inline float grayscale() const {
    return (0.3 * r) + (0.59 * g) + (0.11 * b);
  }

  inline uint32_t to_rgba() const {
    uint32_t a_ = static_cast<uint32_t>(a * 255.0);
    uint32_t r_ = static_cast<uint32_t>(r * 255.0);
    uint32_t g_ = static_cast<uint32_t>(g * 255.0);
    uint32_t b_ = static_cast<uint32_t>(b * 255.0);

    return static_cast<uint32_t>(((r_ & 0xff) << 24) | ((g_ & 0xff) << 16) | ((b_ & 0xff) << 8) | (a_ & 0xff));
  }

  inline static colour from_rgba(uint32_t rgba) {
    float r = ((rgba & 0xFF000000) >> 24) / 255.0;
    float g = ((rgba & 0x00FF0000) >> 16) / 255.0;
    float b = ((rgba & 0x0000FF00) >> 8) / 255.0;
    float a = (rgba & 0x000000FF) / 255.0;
    return colour(a, r, g, b);
  }

  inline uint32_t to_abgr() const {
    uint32_t a_ = static_cast<uint32_t>(a * 255.0);
    uint32_t r_ = static_cast<uint32_t>(r * 255.0);
    uint32_t g_ = static_cast<uint32_t>(g * 255.0);
    uint32_t b_ = static_cast<uint32_t>(b * 255.0);

    return static_cast<uint32_t>((a_ << 24) | (b_ << 16) | (g_ << 8) | (r_ & 0xff));
  }

  inline static colour from_abgr(uint32_t abgr) {
    float a = ((abgr & 0xFF000000) >> 24) / 255.0;
    float b = ((abgr & 0x00FF0000) >> 14) / 255.0;
    float g = ((abgr & 0x0000FF00) >> 8) / 255.0;
    float r = (abgr & 0x000000FF) / 255.0;
    return colour(a, r, g, b);
  }

  inline uint32_t to_argb() const {
    uint32_t a_ = static_cast<uint32_t>(a * 255.0);
    uint32_t r_ = static_cast<uint32_t>(r * 255.0);
    uint32_t g_ = static_cast<uint32_t>(g * 255.0);
    uint32_t b_ = static_cast<uint32_t>(b * 255.0);

    return static_cast<uint32_t>((a_ << 24) | (r_ << 16) | (g_ << 8) | (b_ & 0xff));
  }

  inline static colour from_argb(uint32_t argb) {
    float a = ((argb & 0xFF000000) >> 24) / 255.0;
    float r = ((argb & 0x00FF0000) >> 16) / 255.0;
    float g = ((argb & 0x0000FF00) >> 8) / 255.0;
    float b = (argb & 0x000000FF) / 255.0;
    return colour(a, r, g, b);
  }

  inline uint32_t to_bgra() const {
    uint32_t a_ = static_cast<uint32_t>(a * 255.0);
    uint32_t r_ = static_cast<uint32_t>(r * 255.0);
    uint32_t g_ = static_cast<uint32_t>(g * 255.0);
    uint32_t b_ = static_cast<uint32_t>(b * 255.0);

    return static_cast<uint32_t>((b_ << 24) | (g_ << 16) | (r_ << 8)
        | (a_ & 0xff));
  }

  inline static colour from_bgra(uint32_t bgra) {
    float b = ((bgra & 0xFF000000) >> 24) / 255.0;
    float g = ((bgra & 0x00FF0000) >> 16) / 255.0;
    float r = ((bgra & 0x0000FF00) >> 8) / 255.0;
    float a = (bgra & 0x000000FF) / 255.0;
    return colour(a, r, g, b);
  }

  static colour WHITE() {
    return fw::colour(1.0f, 1.0f, 1.0f, 1.0f);
  }
  static colour BLACK() {
    return fw::colour(1.0f, 0.0f, 0.0f, 0.0f);
  }
};

inline bool operator ==(fw::colour const &lhs, fw::colour const &rhs) {
  // This comparison is only accurate for colours in 256 steps. that is, a difference of 0.001 (for example) will be
  // considered "equal".
  return (lhs.to_argb() == rhs.to_argb());
}

inline bool operator !=(fw::colour const &lhs, fw::colour const &rhs) {
  // This comparison is only accurate for colours in 256 steps. that is, a difference of 0.001 (for example) will be
  // considered "equal"
  return (lhs.to_argb() != rhs.to_argb());
}

inline std::ostream &operator <<(std::ostream &str, fw::colour const &col) {
  str << boost::format("[a=%1$.3f, r=%2$.3f, g=%3$.3f, b=%4$.3f]") % col.a % col.r % col.g % col.b;
  return str;
}

}
