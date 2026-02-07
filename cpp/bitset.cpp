#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <limits>
#include <numeric>
#include <type_traits>

template<std::size_t NBits>
class Bitset {
  using underlying_t = std::conditional_t<(NBits > 32), uint64_t,
                       std::conditional_t<(NBits > 16), uint32_t,
                       std::conditional_t<(NBits > 8),  uint16_t,
                                                        uint8_t>>>;

  static constexpr auto bits_per_word = std::numeric_limits<underlying_t>::digits;
  static constexpr auto all_ones = std::numeric_limits<underlying_t>::max();
  static constexpr auto all_zero = 0;

  static constexpr std::size_t arr_size = (NBits + 63) / 64;
  using storage_t = std::array<underlying_t, arr_size>;
  static constexpr std::size_t bits_in_storage = sizeof(storage_t) * std::numeric_limits<uint8_t>::digits;

 public:
  constexpr Bitset() = default;

  constexpr Bitset(underlying_t bits) requires(NBits <= 64) : bits({bits}) {
    get_msb_word() &= get_msb_mask();
  }

  // Constructor which sets the bits in reading order, i.e.
  // {MSB, ..., LSB}
  constexpr Bitset(storage_t bits) requires(NBits > 64) : bits(bits) {
    std::ranges::reverse(this->bits); // TODO: not really necessary, I can just begin iterating from the last element
    get_msb_word() &= get_msb_mask();
  }

  [[nodiscard]] constexpr auto all() const -> bool {
    return std::ranges::all_of(bits, [](const underlying_t word) -> bool { return word == all_ones; });
  }

  [[nodiscard]] constexpr auto any() const -> bool {
    return std::ranges::any_of(bits, [](const underlying_t word) -> bool { return word != all_zero; });
  }

  [[nodiscard]] constexpr auto none() const -> bool {
    return std::ranges::all_of(bits, [](const underlying_t word) -> bool { return word == all_zero; });
  }

  [[nodiscard]] constexpr auto count() const -> std::size_t {
    return std::accumulate(bits.begin(), bits.end(), std::size_t{0},
        [](std::size_t acc, const underlying_t word) -> std::size_t { return acc + std::popcount(word); });
  }

  [[nodiscard]] constexpr auto size() const -> std::size_t {
    return NBits;
  }

  [[nodiscard]] constexpr auto test(std::size_t pos) const -> bool {
    return to_bool(get_word(pos) >> which_bit(pos));
  }

  constexpr auto set() -> Bitset& {
    std::ranges::for_each(bits, [](underlying_t &word) -> void { word = all_ones; });
    get_msb_word() &= get_msb_mask();
    return *this;
  }

  constexpr auto set(std::size_t pos, bool value = true) -> Bitset& {
    if (value)
      get_word(pos) |= mask_bit(pos);
    else
      get_word(pos) &= ~mask_bit(pos);

    return *this;
  }

  constexpr auto reset() -> Bitset& {
    std::ranges::for_each(bits, [](underlying_t &word) -> void { word = all_zero; });
    return *this;
  }

  constexpr auto reset(std::size_t pos) -> Bitset& {
    set(pos, false);
    return *this;
  }

  constexpr auto flip() -> Bitset& {
    std::ranges::for_each(bits, [](underlying_t &word) -> void { word = ~word; });
  }

  constexpr auto flip(std::size_t pos) -> Bitset& {
    get_word(pos) ^= mask_bit(pos);
    return *this;
  }

  [[nodiscard]] constexpr auto find_first_one() const -> std::size_t {
    for (std::size_t i = 0; i < bits.size(); i++) {
      const auto word = bits[i];
      if (word != all_zero)
        return std::countr_zero(word) + (i * bits_per_word);
    }

    return size();
  }

  [[nodiscard]] constexpr auto find_first_zero() const -> std::size_t {
    for (std::size_t i = 0; i < bits.size(); i++) {
      const auto word = bits[i];
      if (word != all_ones)
        return std::countr_one(word) + (i * bits_per_word);
    }

    return size();
  }

  [[nodiscard]] constexpr static auto get_msb_mask() -> underlying_t {
    if constexpr (NBits == bits_in_storage) {
      return all_ones;
    } else {
      return (1 << (NBits % bits_per_word)) - 1;
    }
  }

 private:
  storage_t bits{0};

  [[nodiscard]] constexpr static auto which_word(std::size_t pos) -> std::size_t {
    return pos / bits_per_word;
  }

  [[nodiscard]] constexpr static auto which_bit(std::size_t pos) -> std::size_t {
    return pos % bits_per_word;
  }

  [[nodiscard]] constexpr static auto mask_bit(std::size_t pos) -> underlying_t {
    return (underlying_t(1) << which_bit(pos));
  }

  [[nodiscard]] constexpr auto get_word(std::size_t pos) -> underlying_t& {
    assert(pos < size());
    return bits[which_word(pos)];
  }

  [[nodiscard]] constexpr auto get_msb_word() -> underlying_t& {
    return bits[arr_size - 1];
  }

  [[nodiscard]] constexpr auto to_bool(underlying_t in) -> bool {
    return in & 0b1;
  }
};

void test() {
  static_assert(sizeof(Bitset<1>) == 1);
  static_assert(sizeof(Bitset<8>) == 1);

  static_assert(sizeof(Bitset<9>)  == 2);
  static_assert(sizeof(Bitset<16>) == 2);

  static_assert(sizeof(Bitset<17>) == 4);
  static_assert(sizeof(Bitset<32>) == 4);

  static_assert(sizeof(Bitset<33>) == 8);
  static_assert(sizeof(Bitset<64>) == 8);

  static_assert(sizeof(Bitset<65>) == 16);
  static_assert(sizeof(Bitset<128>) == 16);

  constexpr Bitset<65> b{{0x1000'0F00'0000'000F, 0xFFFF'FFFF'FFFF'FFFF}};
  static_assert(b.count() == 65);
  static_assert(Bitset<8>::get_msb_mask() == 0xFF);
  static_assert(Bitset<9>::get_msb_mask() == 0x01'FF);
  static_assert(Bitset<15>::get_msb_mask() == 0x7F'FF);
  static_assert(Bitset<16>::get_msb_mask() == 0xFF'FF);
  static_assert(Bitset<64>::get_msb_mask() == 0xFFFF'FFFF'FFFF'FFFF);
  static_assert(Bitset<65>::get_msb_mask() == 0x0001);

  constexpr Bitset<64> c{ 0x0FFF'FFFF'FFFF'FFFF};
  static_assert(c.count() == 60);
}

