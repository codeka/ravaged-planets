#include <framework/packet_buffer.h>

namespace fw {
namespace net {

PacketBuffer::PacketBuffer(uint16_t packet_type) :
    flushed_(false) {
  (*this) << packet_type;
}

PacketBuffer::PacketBuffer(char const *bytes, std::size_t n) :
    buffer_(std::string(bytes, n)), flushed_(false) {
  (*this) >> packet_type_;
}

PacketBuffer::~PacketBuffer() {
}

void PacketBuffer::add_bytes(char const *bytes, std::size_t offset, std::size_t n) {
  flushed_ = false;
  buffer_.write(bytes + offset, n);
}

void PacketBuffer::get_bytes(char *bytes, std::size_t offset, std::size_t n) {
  buffer_.read(bytes + offset, n);
}

void PacketBuffer::flush() {
  if (flushed_)
    return;

  buffer_.flush();
  value_ = buffer_.str();
  flushed_ = true;
}

char const *PacketBuffer::get_buffer() {
  flush();
  return value_.c_str();
}

std::size_t PacketBuffer::get_size() {
  flush();
  return value_.length();
}

}
}
