#ifndef _PURRR_BUFFER_HPP_
#define _PURRR_BUFFER_HPP_

#include "purrr/object.hpp"
namespace purrr {

enum class BufferType {
  Vertex,
  Index
};

struct BufferInfo {
  BufferType type;
  size_t     size;
};

class Buffer : public purrr::Object {
public:
  Buffer()          = default;
  virtual ~Buffer() = default;
public:
  Buffer(const Buffer &)            = delete;
  Buffer &operator=(const Buffer &) = delete;
public:
  virtual void copy(void *data, size_t offset, size_t size) = 0;
};

} // namespace purrr

#endif // _PURRR_BUFFER_HPP_