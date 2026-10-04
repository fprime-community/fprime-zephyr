// Mock Fw::Buffer: pointer + size
#ifndef FW_BUFFER_HPP
#define FW_BUFFER_HPP
#include <Fw/FPrimeBasicTypes.hpp>
namespace Fw {
class Buffer {
  public:
    typedef FwSizeType SizeType;
    Buffer() : m_data(nullptr), m_size(0) {}
    Buffer(U8* data, SizeType size) : m_data(data), m_size(size) {}
    U8* getData() const { return m_data; }
    SizeType getSize() const { return m_size; }
    void setSize(SizeType size) { m_size = size; }

  private:
    U8* m_data;
    SizeType m_size;
};
}  // namespace Fw
#endif
