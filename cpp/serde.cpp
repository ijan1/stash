#include <cassert>
#include <print>
#include <span>
#include <type_traits>
#include <vector>

#if defined(__has_cpp_attribute)
 #if __has_cpp_attribute(clang::lifetimebound)
  #define lifetimebound [[clang::lifetimebound]]
 #else
  #define lifetimebound
 #endif
#else
 #define lifetimebound
#endif

using ByteVec = std::vector<std::byte>;
using ByteSpan = std::span<const std::byte>;
using MutByteSpan = std::span<std::byte>;

template<typename T>
constexpr auto fromBytesTo(ByteSpan bytes) -> T
{
  auto arr = std::array<std::byte, sizeof(T)>{};
  std::copy(bytes.begin(), bytes.end(), arr.begin());
  return std::bit_cast<T>(arr);
}

template<typename T>
constexpr auto toBytes(const T &t) -> std::array<std::byte, sizeof(T)>
{
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(t);
}

template<typename T>
requires std::is_trivially_copyable_v<T>
constexpr auto asBytes(const T &t lifetimebound) -> ByteSpan
{
  return std::as_bytes(std::span(&t, 1));
}

template<typename T>
constexpr auto asBytes(const T &&t) -> ByteSpan = delete;

template<typename T>
requires std::is_trivially_copyable_v<T>
constexpr auto asMutBytes(T &t lifetimebound) -> MutByteSpan
{
  return std::as_writable_bytes(std::span(&t, 1));
}

template<typename T>
constexpr auto asMutBytes(T &&t) -> MutByteSpan = delete;

class ByteBuffer {
 public:
  ByteBuffer(std::size_t size = 128)
  {
    bytes_.reserve(size);
  }

  ByteBuffer(ByteSpan b) : bytes_(b.begin(), b.end()) { }

  ByteBuffer(ByteVec b) : bytes_(std::move(b)) { }

  auto bytes() const lifetimebound -> ByteSpan
  {
    return bytes_;
  }

  auto size() const -> std::size_t
  {
    return bytes_.size();
  }

  void clear()
  {
    bytes_.clear();
  }

  std::size_t write(ByteSpan bytes)
  {
    const auto old_size = bytes_.size();
    std::copy(bytes.begin(), bytes.end(), std::back_inserter(bytes_));
    return bytes_.size() - old_size;
  }

 private:
  ByteVec bytes_;
};

class OByteStream {
 public:
  OByteStream(ByteBuffer &b) : buffer_(b) { }

  template<typename T>
  void write(const T &t)
  {
    write(asBytes(t));
  }

  void write(ByteSpan bytes)
  {
    buffer_.write(bytes);
  }

  auto operator<<(const auto &t) -> OByteStream &
  {
    write(t);
    return *this;
  }

 private:
  ByteBuffer &buffer_;
};

class IByteStream {
 public:
  IByteStream(ByteSpan bytes) : begin_(bytes.data()), current_(bytes.data()), end_(bytes.data() + bytes.size()) { }

  IByteStream(ByteBuffer &buf) : IByteStream(buf.bytes()) { }

  template<typename T>
  requires std::is_trivially_copyable_v<T>
  auto operator>>(T &t) -> IByteStream &
  {
    assert((current_ + sizeof(T)) <= end_);

    t = fromBytesTo<T>({current_, sizeof(T)});
    current_ += sizeof(T);

    return *this;
  }

 private:
  const std::byte *begin_{nullptr};
  const std::byte *current_{nullptr};
  const std::byte *end_{nullptr};
};

int main(int argc, char *argv[])
{
  ByteBuffer buf;
  OByteStream ostream{buf};

  ostream << 10 << 0.5F << 0x1122334455667788UL;

  IByteStream istream{buf};
  int x;
  float y;
  size_t z;
  istream >> x >> y >> z;

  std::println("{} {} {:#x}", x, y, z);
}
