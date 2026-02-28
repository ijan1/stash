#include <algorithm>
#include <cassert>
#include <print>

auto utf8_sequence_length(unsigned char lead_byte) -> size_t {
  if (lead_byte <= 127) return 1;
  if (lead_byte <= 128 + 63) return 2;
  if (lead_byte <= 128 + 64 + 31) return 3;
  if (lead_byte <= 128 + 64 + 32 + 15) return 4;

  return 0;
}

template <typename T>
struct Cursor {
  const T *begin_ = nullptr;
  const T *end_   = nullptr;

  Cursor() : begin_(nullptr), end_(nullptr) {}

  Cursor(std::span<const T> span) : begin_(span.data()), end_(span.data() + span.size()) {}

  constexpr auto operator[](size_t index) -> const T & { return begin_[index]; }

  constexpr auto next() -> const T & {
    const T &ret = *begin_;
    ++begin_;
    assert(begin_ <= end_);
    return ret;
  }

  [[nodiscard]] constexpr auto remaining() const -> size_t { return end_ - begin_; }

  [[nodiscard]] constexpr auto ended() const -> bool { return remaining() == 0; }
};

template <typename Container>
concept Streamable =
    std::contiguous_iterator<typename Container::iterator> && requires(Container &t) {
      { t.begin() };
      { t.end() };
    };

bool decode_glyph(char32_t &in, Cursor<char> &c) {
  if (c.remaining() == 0) {
    in = U'�';
    return false;
  }

  unsigned char b1 = c.next();

  if (utf8_sequence_length(b1) == 0) {
    in = U'�';
    return false;
  }
  if (utf8_sequence_length(b1) > c.remaining() + 1) {
    in = U'�';
    return false;
  }

  // clang-format off
  if (b1 <= 127) {
    in = b1;
  }
  else if (b1 <= 128 + 63) {
    in = ((0x1f & b1) << 6) |
         ((0x3f & c.next()));
  }
  else if (b1 <= 128 + 64 + 31) {
    auto b2 = c.next(); // Since evaluation order is unspecified, we need
    auto b3 = c.next(); // to make sure we grab the correct bytes
    in = ((0x0f & b1) << 12) |
         ((0x3f & b2) << 6)  |
         ((0x3f & b3));
  }
  else if (b1 <= 128 + 64 + 32 + 15) {
    auto b2 = c.next(); // Workaround for unspecified evaluation order
    auto b3 = c.next();
    auto b4 = c.next();
    in = ((0x07 & b1) << 18) |
         ((0x3f & b2) << 12) |
         ((0x3f & b3) << 6)  |
         ((0x3f & b4));
  }  // clang-format on

  return true;
}

template <Streamable S, typename Next>
struct Stream {
  using Base = std::remove_cvref_t<decltype(*std::declval<S>().begin())>;
  using Item = std::invoke_result_t<Next, Cursor<Base> &>;

  using ValueType = std::remove_cvref_t<decltype(*std::declval<Item>())>;

  Cursor<Base> cursor_;
  Next next_;

  Stream(S s, Next &&n) : cursor_(s), next_(std::move(n)) {}

  constexpr auto next() { return next_(cursor_); }

  struct It {
    using iterator_category = std::input_iterator_tag;
    using value_type        = ValueType;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const value_type *;
    using reference         = const value_type &;

    Item current;
    Stream *s;

    constexpr auto operator*() const -> reference { return *current; }
    constexpr auto operator->() const -> pointer { return &*current; }

    constexpr auto operator*() -> reference { return *current; }
    constexpr auto operator->() -> pointer { return &*current; }

    constexpr auto operator++() -> It & {
      current = s->next();
      return *this;
    }

    constexpr auto operator!=(std::nullptr_t) { return current != nullptr; }
    constexpr auto operator!=(std::nullopt_t) { return current != std::nullopt; }
  };  // struct It

  constexpr auto begin() -> It { return It{next(), this}; }

  constexpr auto end() {
    if constexpr (std::is_pointer_v<Item>) { return nullptr; }
    else { return std::nullopt; }
  }

  // TODO: enumerate -> std::pair(size_t, Item)
};

constexpr auto iterGraphemes(std::string_view str) {
  return Stream(
      str,
      [](auto &c) -> std::optional<char32_t>
      {
        char32_t in;
        return decode_glyph(in, c) ? std::optional(in) : std::nullopt;
      }
  );
}

constexpr auto iterChars(std::string_view str) {
  return Stream(
      str,
      [](auto &c) -> const char *
      {
        return !c.ended() ? &c.next() : nullptr;
      }
  );
}

int main() {
  std::string test = "Hello World, from Japan and други места.\n";

  size_t len = 0;
  for (auto ch : iterGraphemes(test)) {
    std::println("ch: {}", (char)ch);
    len++;
  }

  for (auto ch : iterChars(test)) {
    std::println("ch: {}", ch);
  }
}

