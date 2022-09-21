#pragma once

#include <sstream>
#include <framework/color.h>
#include <framework/vector.h>

namespace fw::net {

// This class represents a "buffer" we use for reading/writing packets that get sent between net_peers.
class PacketBuffer {
private:
  bool flushed_;
  std::string value_;
  std::stringstream buffer_;
  uint16_t packet_type_;

  void flush();

public:
  PacketBuffer(uint16_t packet_type);
  PacketBuffer(char const *bytes, std::size_t n);
  ~PacketBuffer();

  void add_bytes(char const *bytes, std::size_t offset, std::size_t n);
  void get_bytes(char *bytes, std::size_t offset, std::size_t n);

  char const *get_buffer();
  std::size_t get_size();
  uint16_t get_packet_type() const {
    return packet_type_;
  }
};

inline PacketBuffer &operator <<(PacketBuffer &lhs, int32_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 4);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, int32_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 4);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, uint32_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 4);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, uint32_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 4);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, int16_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 2);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, int16_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 2);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, uint16_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 2);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, uint16_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 2);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, int64_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 8);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, int64_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 8);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, uint64_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 8);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, uint64_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 8);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, uint8_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, 1);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, uint8_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, 1);
  return lhs;
}

#ifdef __APPLE__
// CLang for Apple makes size_t a different type, but on other platforms, it's the same as uint64_t (or uint32_t).

inline packet_buffer &operator <<(packet_buffer &lhs, size_t rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(&rhs), 0, sizeof(size_t));
  return lhs;
}

inline packet_buffer &operator >>(packet_buffer &lhs, size_t &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(&rhs), 0, sizeof(size_t));
  return lhs;
}

#endif

inline PacketBuffer &operator <<(PacketBuffer &lhs, fw::Vector const &rhs) {
  lhs.add_bytes(reinterpret_cast<char const *>(rhs.data()), 0, sizeof(float) * 3);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, fw::Vector &rhs) {
  lhs.get_bytes(reinterpret_cast<char *>(rhs.data()), 0, sizeof(float) * 3);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, Color const &rhs) {
  uint32_t rgba = rhs.to_rgba();
  lhs.add_bytes(reinterpret_cast<char const *>(&rgba), 0, sizeof(uint32_t));
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, Color &rhs) {
  uint32_t rgba;
  lhs.get_bytes(reinterpret_cast<char *>(&rgba), 0, sizeof(uint32_t));
  rhs = fw::Color(rgba);
  return lhs;
}

inline PacketBuffer &operator <<(PacketBuffer &lhs, std::string const &rhs) {
  // strings are length-prefixed
  uint16_t length = static_cast<uint16_t>(rhs.length());
  lhs << length;
  lhs.add_bytes(rhs.c_str(), 0, length);
  return lhs;
}

inline PacketBuffer &operator >>(PacketBuffer &lhs, std::string &rhs) {
  uint16_t length;
  lhs >> length;

  if (length < 80) { // If it's < 80 bytes, just allocate on the stack.
    char *ParticleRotation = reinterpret_cast<char *>(alloca(length));
    lhs.get_bytes(ParticleRotation, 0, length);
    rhs = std::string(ParticleRotation, length);
  } else {
    char *ParticleRotation = new char[length];
    lhs.get_bytes(ParticleRotation, 0, length);
    rhs = std::string(ParticleRotation, length);
    delete[] ParticleRotation;
  }

  return lhs;
}

}
