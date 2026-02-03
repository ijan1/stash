// g++ -std=c++23 myvariant.cpp
#include <cstdio>
#include <iostream>
#include <utility>
#include <variant>

template<typename ...Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};

struct TypeTeller
{
    void operator()(this auto&& self)
    {
        using SelfType = decltype(self);
        using UnrefSelfType = std::remove_reference_t<SelfType>;
        if constexpr (std::is_lvalue_reference_v<SelfType>)
        {
            if constexpr (std::is_const_v<UnrefSelfType>)
                std::cout << "const lvalue\n";
            else
                std::cout << "mutable lvalue\n";
        }
        else
        {
            if constexpr (std::is_const_v<UnrefSelfType>)
                std::cout << "const rvalue\n";
            else
                std::cout << "mutable rvalue\n";
        }
    }
};


struct Tracer {
  Tracer() noexcept
  {
    std::puts("MyType()");
  }

  Tracer(const char *) noexcept
  {
    std::puts("MyType(const char *)");
  }

  Tracer(const Tracer &) noexcept
  {
    std::puts("MyType(const MyType &)");
  }

  Tracer(Tracer &&) noexcept
  {
    std::puts("MyType(MyType &&)");
  }

  Tracer &operator=(Tracer &&) noexcept
  {
    std::puts("operator=(MyType &&)");
    return *this;
  }

  Tracer &operator=(const Tracer &) noexcept
  {
    std::puts("operator=(const MyType &)");
    return *this;
  }

  ~Tracer()
  {
    std::puts("~Tracer()");
  };
};

template<typename Variant, typename T, std::size_t index = 0>
consteval auto get_index() -> std::size_t
{
  if constexpr (std::is_same_v<T, std::variant_alternative_t<index, Variant>>) {
    return index;
  }
  else {
    return get_index<Variant, T, index + 1>();
  }
};

// Variant MixIn
template<typename... Ts>
struct MyVariant : std::variant<Ts...> {
 public:
  using Base = std::variant<Ts...>;
  using Base::Base;

  MyVariant() = default;

  // allows us to get the index from a type
  // MyVariant::get_index<int>();
  template<typename T>
  constexpr static auto get_index() -> std::size_t
  {
    return ::get_index<Base, T>();
  }

  template<typename T>
  constexpr static auto Create(auto &&...args) -> MyVariant
  {
    return MyVariant{std::in_place_type<T>, std::forward<decltype(args)>(args)...};
  }

#if (__cpp_lib_variant < 202306L)
  template<class Visitor>
  constexpr decltype(auto) visit(this auto &&self, Visitor &&vis)
  {
	  using CVar = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>,
					const Base, Base>;
	  using Var = std::conditional_t<std::is_rvalue_reference_v<decltype(self)&&>,
				       CVar&&, CVar&>;
    // TODO: using 'V' doesn't do the correct thing when && and const &&
    using V = decltype(std::forward_like<decltype(self)&&>(std::declval<Base>()));
    return std::visit(std::forward<Visitor>(vis), static_cast<Var>(self));
  }

  template<class R, class Visitor>
  constexpr R visit(this auto &&self, Visitor &&vis)
  {
	  using CVar = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>,
					const Base, Base>;
	  using Var = std::conditional_t<std::is_rvalue_reference_v<decltype(self)&&>,
				       CVar&&, CVar&>;
    // TODO: using 'V' doesn't do the correct thing when && and const &&
    using V = decltype(std::forward_like<decltype(self)&&>(std::declval<Base>()));
    return std::visit<R>(std::forward<Visitor>(vis), static_cast<Var>(self));
  }
#endif

};

struct Flee {
  void operator()()  &{
    std::puts("&");
  }
  void operator()() const& {
    std::puts("const&");
  }
  void operator()()  const&&{
    std::puts("const&&");
  }
  void operator()()  &&{
    std::puts("&&");
  }
};

struct Fight {
  void operator()() const {}
};

using Action = MyVariant<Flee, Fight, TypeTeller>;

void test() {
  Action a = Action::Create<TypeTeller>();

  const auto visitor = [](auto &&val) {
    val();
  };

  a.visit(visitor);
  std::as_const(a).visit(visitor);
  std::move(a).visit(visitor);
  std::move(std::as_const(a)).visit(visitor);
}
