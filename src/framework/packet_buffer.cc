#include <framework/packet_buffer.h>

namespace fw {
namespace net {

packet_buffer::packet_buffer(uint16_t packet_type) :
    _flushed(false) {
  (*this) << packet_type;
}

packet_buffer::packet_buffer(char const *bytes, std::size_t n) :
    _buffer(std::string(bytes, n)), _flushed(false) {
  (*this) >> _packet_type;
}

packet_buffer::~packet_buffer() {
}

void packet_buffer::add_bytes(char const *bytes, std::size_t offset, std::size_t n) {
  _flushed = false;
  _buffer.write(bytes + offset, n);
}

void packet_buffer::get_bytes(char *bytes, std::size_t offset, std::size_t n) {
  _buffer.read(bytes + offset, n);
}

void packet_buffer::flush() {
  if (_flushed)
    return;

  _buffer.flush();
  _value = _buffer.str();
  _flushed = true;
}

char const *packet_buffer::get_buffer() {
  flush();
  return _value.c_str();
}

std::size_t packet_buffer::get_size() {
  flush();
  return _value.length();
}

}
}
