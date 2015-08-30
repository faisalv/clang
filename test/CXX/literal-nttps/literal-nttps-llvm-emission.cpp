// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s

//expected-no-diagnostics

namespace test_lvalue_emission {
union U {
  int J = 5;
  int I;
};

template<U u> 
struct X { 
  int foo() {
    int I = u.I;
    constexpr int J2 = u.J;
    return u.J + J2;
  }
  int foo2() {
    return u.J;
  }
};



int main() {
  X<{}> xu;
  xu.foo();
  xu.foo2();
  return 0;
}

namespace ns2 {
union U {
  int J = 5;
  int I;
};


template<class T> struct Y { };


template<U u> 
struct X { 
  constexpr U foo() {
    return u;
  }
};

template<U u, template<U> class XX> struct Y<XX<u>> {
  constexpr U foo() { return u; }
};

int main() {
  X<{}> xu;
  constexpr auto uu = xu.foo();
  Y<X<{}>> yxu;
  constexpr auto yuu = yxu.foo();
  return uu.I;
}
} // end ns2

namespace ns3 {


union U {
  int J = 5;
  int I;
};

template<class T> struct Y { };


template<U u> 
struct X { 
  constexpr U foo() {
    return u;
  }
};

template<U u, template<U> class XX> struct Y<XX<u>> {
  constexpr U foo() { return u; }
};

template<template<U> class XX> struct Y<XX<{7}>> {
  constexpr int goo() { return 3; }
};

int main() {
  X<{}> xu;
  constexpr auto uu = xu.foo();
  Y<X<{}>> yxu;
  Y<X<U{7}>>{}.goo();
  constexpr auto yuu = yxu.foo();
  return uu.I + yuu.I;
}

} // end ns3 
} // end test_lvalue_emission

namespace test_range_deduction {
namespace ns1 {

template<class T, int N> struct Range { 
  T Vec[N];
  enum { Size = N };
  using type = T;
  constexpr T const *begin() const {
    return Vec;
  }
  constexpr T const *end() const { return Vec + N; }
};

template<class R, R Range> struct Wrapper { 
  static constexpr R get() { return Range; }
};

/*
template<class T, int N, template<class, int> class RangeTT, RangeTT<T, N> R> auto foo(Wrapper<RangeTT<T, N>, R> w) {
  return w;
}
*/

template<class R> constexpr R reverse_impl(R r) {
   R ret{};
   for (unsigned I = 0; I < R::Size; ++I)
     ret.Vec[I] = r.Vec[R::Size - 1 - I];
   return ret;
} 

template<class R, R Range> constexpr auto reverse(Wrapper<R, Range> w) {
  constexpr auto ReverseRange = reverse_impl(Range);
  // Just to show, you can use it as a template argument...
  Wrapper<R, ReverseRange> w2;
  
  return w2.get();
}

template<> constexpr auto reverse(Wrapper<const Range<int, 6>, {1, 2, 3, 4, 5, 6}> w) {
  return Range<int, 6>{-1, -1, 4, 3, 2, 1};
}

constexpr auto reverse(Wrapper<const Range<float, 7>, {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7}> w) {
  return Range<float, 7>{-1.1, -2.2, -3.3, -4.4, -5.5, -6.6, -7.7};
}

template<int N>
constexpr auto reverse(Wrapper<const Range<int, N>, Range<int, N>{1}> w) {
  return Range<int, N>{-100, -200};
}

int main() {
  constexpr Range<char, 10> R{"123456789"};
  Wrapper<decltype(R), R> w;

  static_assert(*(reverse(w).begin() + 1) == '9', "");
  // Make sure it uses the specialization
  constexpr Range<int, 6> RI{1, 2, 3, 4, 5, 6};
  Wrapper<decltype(RI), RI> w2;
  constexpr int I = *(reverse(w2).begin() + 1);
  static_assert(I == -1, "");  
  {
    using type = float;
    constexpr int N = 7;
    constexpr Range<type, N> RI{1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7};
    Wrapper<decltype(RI), RI> w2;
    constexpr type I = *(reverse(w2).begin() + 1);
    static_assert(I == -2.2F, "");  
  }
  {
    using type = float;
    constexpr int N = 8;
    constexpr Range<type, N> RI{1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7};
    Wrapper<decltype(RI), RI> w2;
    constexpr type I = *(reverse(w2).begin() + 1);
    static_assert(I == 7.7F, "");  
  }
  {
    using type = int;
    constexpr int N = 7;
    constexpr Range<type, N> RI{1, 2, 3, 4, 5, 6};
    Wrapper<decltype(RI), RI> w2;
    constexpr type I = *(reverse(w2).begin() + 1);
    static_assert(I == 6, "");  
  }
  {
    using type = int;
    constexpr int N = 7;
    constexpr Range<type, N> RI{1};
    Wrapper<decltype(RI), RI> w2;
    constexpr type I = *(reverse(w2).begin() + 1);
    static_assert(I == -200, "");  
  }
  return 0;
}
} //end ns1
namespace ns2 {
template<class T, int N> struct Range { 
  T Vec[N];
  constexpr T const *begin() const {
    return Vec;
  }
  constexpr T const *end() const { return Vec + N; }
};

template<Range<char,10> R> constexpr Range<char, 10> reverse() {
   constexpr const char C = *R.begin();
   const char *P = R.begin();
   return R;
}

template<> constexpr Range<char, 10> reverse<{"12345"}>() {
  return Range<char,10>{"54321"};
}  

int main() {
  static_assert(*reverse<{"12345"}>().begin() == '5', "");    
  static_assert(*reverse<{"123456"}>().begin() == '1', "");    
  return 0;
}

} // end ns2
} // end test_range_deduction
