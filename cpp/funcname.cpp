#include <print>
#include <string_view>
#include <source_location>

template<typename T>
consteval auto functionName() -> std::string_view {
  return std::source_location::current().function_name();
}

template<typename T>
auto nameOf() -> std::string_view {
  constexpr auto name = functionName<T>();

  constexpr auto prefix = std::string_view{"T = "};
  constexpr auto prefix_len = name.find(prefix) + prefix.size();

  constexpr auto suffix_len = functionName<void>().size() - prefix_len - 4;

  constexpr auto length = name.size() - prefix_len - suffix_len;

  return name.substr(prefix_len, length);
}

template<>
auto nameOf<uint32_t>() -> std::string_view {
  return "uint32_t";
}

struct Meow {};
struct BauBau {};
class Bau {};

int main() {
  std::print("{}\n", nameOf<void>());
  std::print("{}\n", nameOf<int>());
  std::print("{}\n", nameOf<Meow>());
  std::print("{}\n", nameOf<BauBau>());
  std::print("{}\n", nameOf<Bau>());
  std::print("{}\n", nameOf<float>());
  std::print("{}\n", nameOf<double>());
  std::print("{}\n", nameOf<uint32_t>());
  std::print("{}\n", nameOf<unsigned int>());
  std::print("{}\n", nameOf<std::size_t>());
}
