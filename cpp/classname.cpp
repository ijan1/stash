// Utility which returns the name of a type as a string
// NOTE: clang returns pointer classes with a space between the name and the asterisk:
// Foo *
// Whereas, gcc does not:
// Foo*

// g++ -std=c++23 classname.out

#include <cstdint>
#include <string_view>
#include <source_location>

template<typename T>
consteval auto nameOf_() -> std::string_view {
  return std::source_location::current().function_name();
}

template<typename T>
consteval auto nameOf() -> std::string_view {
  constexpr auto name = nameOf_<T>();

  constexpr auto prefix = std::string_view{"T = "}; // TODO: MSVC prefix
  constexpr auto prefix_len = name.find(prefix) + prefix.size();

  constexpr auto suffix_len = nameOf_<void>().size() - prefix_len - 4;

  constexpr auto length = name.size() - prefix_len - suffix_len;

  return name.substr(prefix_len, length);
}

template<>
consteval auto nameOf<void>() -> std::string_view {
  return "void";
}
// chars
template<>
consteval auto nameOf<char>() -> std::string_view {
  return "char";
}
template<>
consteval auto nameOf<char8_t>() -> std::string_view {
  return "char8_t";
}
template<>
consteval auto nameOf<char16_t>() -> std::string_view {
  return "char16_t";
}
template<>
consteval auto nameOf<char32_t>() -> std::string_view {
  return "char32_t";
}
// integers
template<>
consteval auto nameOf<uint8_t>() -> std::string_view {
  return "uint8_t";
}
template<>
consteval auto nameOf<uint16_t>() -> std::string_view {
  return "uint16_t";
}
template<>
consteval auto nameOf<uint32_t>() -> std::string_view {
  return "uint32_t";
}
template<>
consteval auto nameOf<uint64_t>() -> std::string_view {
  return "uint64_t";
}
template<>
consteval auto nameOf<int8_t>() -> std::string_view {
  return "int8_t";
}
template<>
consteval auto nameOf<int16_t>() -> std::string_view {
  return "int16_t";
}
template<>
consteval auto nameOf<int32_t>() -> std::string_view {
  return "int32_t";
}
template<>
consteval auto nameOf<int64_t>() -> std::string_view {
  return "int64_t";
}

// floating point
template<>
consteval auto nameOf<float>() -> std::string_view {
  static_assert(sizeof(float) == 4);
  return "f32";
}
template<>
consteval auto nameOf<double>() -> std::string_view {
  static_assert(sizeof(double) == 8);
  return "f64";
}

struct Meow { };
struct BauBau {};
class Bau {};

namespace Foo {
  class Bar{};
}

void test() {
  using namespace std::string_view_literals;

  static_assert(nameOf<void>() == "void"sv);

  static_assert(nameOf<char>()     == "char"sv);
  static_assert(nameOf<char8_t>()  == "char8_t"sv);
  static_assert(nameOf<char16_t>() == "char16_t"sv);
  static_assert(nameOf<char32_t>() == "char32_t"sv);
  static_assert(nameOf<wchar_t>()  == "wchar_t"sv);

  static_assert(nameOf<uint8_t>()  == "uint8_t"sv);
  static_assert(nameOf<uint16_t>() == "uint16_t"sv);
  static_assert(nameOf<uint32_t>() == "uint32_t"sv);
  static_assert(nameOf<uint64_t>() == "uint64_t"sv);
  static_assert(nameOf<int8_t>()   == "int8_t"sv);
  static_assert(nameOf<int16_t>()  == "int16_t"sv);
  static_assert(nameOf<int32_t>()  == "int32_t"sv);
  static_assert(nameOf<int64_t>()  == "int64_t"sv);

  static_assert(nameOf<float>()  == "f32"sv);
  static_assert(nameOf<double>() == "f64"sv);

  static_assert(nameOf<Meow>()   == "Meow"sv);
  static_assert(nameOf<BauBau>() == "BauBau"sv);
  static_assert(nameOf<Bau>()    == "Bau"sv);

  static_assert(nameOf<Foo::Bar>() == "Foo::Bar"sv);
}

// Helper function so that you don't have to specify template
template<typename T>
consteval auto nameOfAuto([[maybe_unused]] const T &arg) -> std::string_view {
  using real_type = std::remove_pointer_t<T>;
  return nameOf<real_type>();
}

struct Test {
  consteval std::string_view ID() {
    return nameOfAuto(this);
  }
};

void test_two() {
  static_assert(Test{}.ID() == "Test");
  static_assert(nameOfAuto(10) == "int32_t");
  static_assert(nameOfAuto(10U) == "uint32_t");
}

