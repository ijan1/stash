#include <utility>

template<typename X, typename ...Args>
struct Base {
 private:
  Base() = default;

 public:
  X f_pre_23() const
  {
    return X{};
  }

  constexpr auto &&f_23(this auto &&self) {
    return std::forward<decltype(self)>(self);
  }

  friend X;
};

template<template<typename> typename T, class ...Args>
struct Derived_ : T<Derived_<T, Args...>>
{};


template<typename ...Args>
using Derived = Derived_<Base, Args...>;

struct Action : Derived<int, double> { };

int main() {
  Action h;
  // 'f_pre_23()' is going to return Derived_, instead of Action
  auto t = h.f_pre_23();
  // Action a = h.f_pre_23(); // doesn't work

  // 'f_23()' returns type Action
  auto a = h.f_23();

};
