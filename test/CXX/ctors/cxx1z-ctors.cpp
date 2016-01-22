// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING


namespace parser_ish_tests {
template<class T = int> struct X { }; //expected-note{{here}}

namespace ns1 {
  void foo(X x) { } //expected-error{{missing template arguments}} 
  
  // FIXME: this error is a little misleading, but OK
  template<X x> struct Y { }; //expected-error{{cannot have type}} expected-error{{missing template arguments}}
  void test() {
    X x;
    X (((y)));
    X x2{};
    X (((y2))){};
   // do not allow non-functional cast notation.
   (X) x3; //expected-error{{requires template arguments}}
   
   X x18{X{}};
   X x19 = X{};
   
   X x20(); //expected-error{{missing template arguments}} \
            //expected-warning{{interpreted as a function declaration}} \
            //expected-note{{replace parentheses}}
   X<int> x25(X<int> p);
   X<int> x26(X p); //expected-error{{missing template arguments}}
   {
     extern X x; //expected-error{{missing template arguments}}
   }
  }
} // end ns1
namespace ns2 {
template<class T> struct X { }; //expected-note3{{candidate}}

X x; //expected-error{{no viable constructor}}

}
namespace ns3 {
template<class ... Ts> struct V { V(Ts...); };
template<class T, class U> struct A { A(T, U); }; //expected-note4{{here}}
struct S {
  V<int> mv;
  A<int> ma; //expected-error{{too few template arguments}}
}; 
namespace test_return_is_invalid {
A foo(); //expected-error{{missing template arguments}}
auto foo() ->A; //expected-error{{requires template arguments}}
A<int> foo2();  //expected-error{{too few template arguments}}
auto foo2()->A<int>; //expected-error{{too few template arguments}}

} // end test_return_is_invalid

namespace test_disallowed_in_template_parameter {
template<class T, class U> struct A { }; //expected-note2{{declared here}}
template<class T> struct B { };
B<A> b; //expected-error{{requires template arguments}}
B<A<int, char>> b2;
B<A<int>> b3; //expected-error{{too few template arguments}}
} // end test_disallowed_in_template_parameter
namespace ns1 {
template<class T, class U> struct A { A(T, U); operator bool() const; };
  void foo() {
    if (A a{1, 2}) { }
    for (A a{"abc", 1}; ;) { }
    if (A<char> a{1, 2}) { }
    for (A<> a{"abc", 1}; ;) { }
    if (A<> a{1, 2}) { }
    for (A<const char*> a{"abc", 1}; ;) { }
  } 
} // end ns1
} // end ns3
namespace ns4 {
template<class T> struct A { };
template<class T> A<T> A(T) noexcept; //expected-error{{exception}}
} // end ns4

namespace ns5 {
template<class T, class U> struct A { A(T, U) { } };


void foo() {
  A<int, const char*> *a1 = new A{1, "abc"};
  A<char, const char*> *a2 = new A<char>{1, "abc"};
}
}// end ns5
} // end ns parser_tests

template<class T, class U> struct is_same_t { enum { value = false }; };
template<class T> struct is_same_t<T, T> { enum { value = true }; };

template<class T, class U> constexpr bool is_same = is_same_t<T, U>::value;
#define IS_SAME(_Var,...) static_assert(is_same<decltype(_Var),__VA_ARGS__>)

namespace test_variadic_deducers {
namespace ns1 {
template<class ... Ts> struct V { V(double, double, double); };

template<class ... Ts> auto V(Ts...) -> V<Ts...>;
template<class ... Ts> V<Ts...> V(Ts...);
V v{1, '2', 3.14};
IS_SAME(v, V<int,char,double>);

} //end ns1
} // end test_variadic_deducers

namespace partial_template_args {

namespace ns1 {
template<class ... Ts> struct V { V(Ts...); };
template<class T, class U> struct A { A(T, U); };  //expected-note{{here}} 

V<int> v{1, '2', 3.14};

IS_SAME(v, V<int, char, double>);
A<int> a{1, "abc"};
IS_SAME(a, A<int, const char *>);
A<int*> a2("abc", "abc"); //expected-error{{too few template arguments}} 
} // end ns1

namespace ns2 {
// check transformation within a template

template<class T, class U> struct A { A(T, double, const void*); A(T,U); };

template<class T, class U> auto f(T t, U u) {
  A<T> a{t, u};
  return a;
}

static_assert(is_same<decltype(f('s', 3.14)),A<char,double>>);

template<class T, class U, class U2> A<T,U> A(T, U, U2);

template<class T, class U, class U2> auto f3(T t, U u, U2 u2) {
  A<T> a{t, u, u2};
  return a;
}

static_assert(is_same<decltype(f3('s', 3.14, "abc")),A<char,double>>);

template<template<class, class> class TT, class T, class U, class U2> auto f4(T t, U u, U2 u2) {
  TT<T> a{t, u, u2};
  return a;
}


static_assert(is_same<decltype(f4<A>('s', 3.14, "abc")),A<char,double>>);


template<template<class...> class TT, class T, class U, class U2> auto f5(T t, U u, U2 u2) {
  TT<T> a{t, u, u2};
  return a;
}
static_assert(is_same<decltype(f5<A>('s', 3.14, "abc")),A<char,double>>);

template<class ... Ts> struct V { V(Ts*...); };


} // end ns2
namespace ns3 {
template<class T> struct A {
  template<class U> struct B { }; //expected-note3{{candidate}}
  template<class T0, class U> typename A<T0>::template B<U> B(T0, U); //expected-error{{return type of a class template deducer}}
};
A<int>::B b{"abc", 3.14};  //expected-error{{missing template arguments}}
} // end ns3

namespace ns4 {

namespace fv {
template<class T, class U> struct A { A(T, U) { } };
}
using namespace ns1;

int main() {
  A<int>{'a', 3};
  fv::A<int>{'a', 3};
}

} // end ns4

} // end partial_template_args


namespace incomplete_template {
template<class T> struct Y;
Y y{2}; //expected-error{{missing template arguments}}

} //end ns incomplete_template

namespace test_deducer_function {
namespace ns1 {

template<class T, int N> struct A {
  T data[N];
};
template<class T, int N> A<T,N> A(const T (&)[N]);

A a({1, 2, 3, 4, 5});
IS_SAME(a, A<int, 5>);
} // end ns1
} // end ns test_deducer_function

namespace test_simple_deduction {
namespace ns1 {
template<class T> struct X {
  T t;
  X(T t) : t(t) { }
};
  
X x{1};
X<int> *cp = (decltype(x)*)0;
} // end ns1

namespace ns2 {
template<class T, class U = T> struct X {
  T t;
  X(T t) : t(t) { }
  X(T t, U u) { }
};
  
X x{1};
X<int,int> *cp = (decltype(x)*)0;
X x2{1, "abc"};
X<int, const char*> *cp2 = (decltype(x2)*)0;
} // end ns2

namespace ns3 {
template<class T, class U, int N, int M, template<class, int> class TT, template<class, int> class UU = TT> struct X {
  X(T, U);
};

template<class T, int N> struct V {
  T d[N];
  V(const T (&a)[N]) {}
};

template<class T = int>
using Arr = T[];
V v(Arr<>{1, 2, 3, 4});
IS_SAME(v,V<int,4>);

V v2(Arr<char>{1, 2, 3, 4});
IS_SAME(v2, V<char, 4>);

V v3(Arr<double>{1, 2, 3, 4});
IS_SAME(v3, V<double, 4>);
IS_SAME(V({1, 2, 3}), V<int, 3>);
decltype(V{{1.1, 2.2, 3.3}}) *vp = (V<double, 3> *) nullptr;
decltype(V({1.1, 2.2, 3.3, 4.4})) *vp2 = (V<double, 4> *) nullptr;
}// end ns3

namespace ns4 {
template<class T, int N> struct X {
  template<class U, template<class, int> class TT, int O = N> X(T, U, TT<T,N>);
};
template<class, int> struct Vec {};
X x{1, "abc", Vec<int, 5>{}};
IS_SAME(x, X<int, 5>);

} // end ns4

namespace ns5 {
template<class T, int N, int M = N, class T2 = T> struct X {
  template<class U, template<class, int> class TT, int O = N> X(T, U, TT<T,N>) { static_assert(O == N); static_assert(M == O); }
};
template<class, int> struct Vec {};
X x{1, "abc", Vec<int, 5>{}};
IS_SAME(x, X<int, 5, 5, int>);

template<class T, int N, int M = N, class T2 = T> struct Y { //expected-note2{{candidate}}
  template<class U, template<class, int> class TT, int O = N> Y(T, U, TT<T,N>, TT<U,M>) {  } //expected-note{{candidate}}
};


template<class T, int N> using TYA = Y<T, N, N+1, T>;

TYA y2{(char*)0,6,Vec<char*, 5>{},Vec<int,6>{}};

// 7 is not 5 + 1 below.
TYA y3{(char*)0,6,Vec<char*, 5>{},Vec<int,7>{}}; //expected-error{{missing template arguments}}


} // end ns5

namespace ns6 {
template<class T, int N, template<class, int> class TT, int M = N, class T2 = T> struct X {
  template<template<class, int> class UU = TT, int O = N, class U> X(T, U, TT<T,N>) { static_assert(O == N); static_assert(M == O); }
};
template<class, int> struct Vec {};
X x{1, "abc", Vec<int, 5>{}};
X xp(1, "abc", Vec<int, 5>{});
IS_SAME(x, X<int, 5, Vec, 5, int>);
decltype(X{1, "abc", Vec<int, 5>{}}) *p1 = &x;
decltype(X(1, "abc", Vec<int, 5>{})) *p2 = &x;
} // end ns6

namespace ns7_check_errors_during_ovl {
template<class T, class U> struct X { //expected-note2{{candidate}}
  X(T, U); //expected-note2{{candidate}}
  X(U, T); //expected-note2{{candidate}}
  X(T, U, T, U) = delete; //expected-note{{deleted}} expected-note{{candidate}}
}; 

X x{1, 'a'}; //expected-error{{ambiguous}}
X y{1, 'a', 3.14}; //expected-error{{no viable}}
X z{"abc", 3.14, "def", 6.26}; //expected-error{{deleted}}

} // end ns7
} // end ns test_simple_deduction

namespace test_type_ctor {
namespace ns1 {
template<typename T> struct X { 
    X(T);
  };
auto x = X{5};
IS_SAME(x, X<int>);
IS_SAME(X{'a'}, X<char>);
IS_SAME(X('a'), X<char>);
} // end ns1

} //end ns type_ctor


namespace test_templates_as_arguments {
namespace ns1 {
template<template<class> class...> struct TT { };

template<class T> struct XX { template<class U> struct YY { YY(U); }; }; //expected-note 3{{candidate}}

TT<XX<int>::YY> mt;
TT<XX<int>::YY{2}> mt2; //expected-error{{must be a class template}}

TT<XX<int>::YY{2, 3}> mt2; //expected-error{{no viable}}
} // end ns1
} // end ns test_templates_as_arguments

namespace test_implicit_constructors {
namespace ns1 {
template<class T = int*> struct X { };
X x{};
IS_SAME(x, X<int*>);
X x2{x};
IS_SAME(x2, X<int*>);
const volatile X cvx{};
IS_SAME(cvx, const volatile X<int*>);
IS_SAME(cvx, volatile const X<int*>);
IS_SAME(cvx, X<int*> volatile const);
IS_SAME(cvx, const X<int*>); //expected-error{{failed}}
const X cx{};
IS_SAME(cx, const X<int*>);
volatile X vx{};
IS_SAME(vx, X<int*> volatile);



} // end ns1

namespace ns2_check_graceful_failure {
template<class T = int*> struct X { X(const X&) = delete; }; //expected-note{{candidate}}
X x{}; //expected-error{{no viable}}
X x2{x}; //expected-error{{invalid initializer}}
} // end ns2
namespace ns3 {
template<class T = int*> struct X { X(X&&); X() = default; };
X x{}; 
X x2{X{}}; 
} // end ns3

} // end ns test_implicit_constructors

namespace test_within_dependent_ctx {

template<class T> struct X { X(T); };

template<class T> struct Y {};
namespace ns1 {
template<class T> auto f(T t) {
  X x{t};
  static_assert(is_same<decltype(x),decltype(X{t})>);
  return x;
}
auto X1 = f((double*)0);
static_assert(is_same<decltype(X1),X<double*>>);
auto X2 = f((int*)0);
static_assert(is_same<decltype(X2),X<int*>>);
auto X3 = f(X2);
static_assert(is_same<decltype(X2),decltype(X3)>, "Implicit copy ctor X(X<T>) is more specialized than X(T)");
auto X4 = f(Y<int*>{});
static_assert(is_same<decltype(X4), X<Y<int*>>>);
} // end ns1

namespace ns2 {
  template<class ... Ts> struct X { X(Ts...); };

  
  template<class T> auto foo(T t) {
    return [=](auto ... a) {
      return [=](auto ... b) {
        return X{t, a..., b...};
      };
    };
  }
  decltype(foo(1)(3.14)('c'))* xp = (X<int, double, char> *) nullptr;
  decltype(foo(1)(3.14, "abc")('c', (float*)0))* xp2 = (X<int, double, const char*, char, float*> *) nullptr;
} // end ns2
} // end ns test_within_dependent_ctx


namespace test_default_args_order {
  template<class T = char> struct X { template<class U> X(U); };  
  X x{3.14};
  IS_SAME(x, X<char>);
}
namespace test_template_template_parameters {
 namespace ns1 {
  template<class T = int> struct X { X(T); };
  
  template<class U, 
           template<class ...> class TT = X> 
  auto f(U u) {
    TT t{u};
    return TT{3};
  }
  
  auto X2 = f(5.12);
  
 } // end ns1
} // end test_template_template_parameters

namespace test_template_alias_deduction {
namespace ns1 {
template<class T, class U> struct X { template<class V> X(T, U, V); }; //expected-note3{{candidate}}
template<class T, class U> using TA2 = int;
template<class T = int> using TA = TA2<T, int>;

TA ta{}; //expected-error{{missing template arguments}}
template<class T> using XTA = X<T, T>;
XTA x{(int*)0, 4, 3.14}; //expected-error{{missing template arguments}}
XTA x2{(int*)0, (int*)0, 3.14}; 

static_assert(is_same<decltype(x2), X<int*, int*>>);

} //end ns1

namespace ns2 {

template<class T> struct X { 
  template<class U> struct Y { };
};

template<class T = int, class U = double> using TA = typename X<T>::template Y<U>;

TA ta{}; //expected-error{{missing template arguments}}

} // end ns2

namespace ns3 {

template<class T> struct X { 
  template<class U> struct Y { };
};
template<class T, class U> using TA2 = X<int>;
template<class T = int> using TA = TA2<T, int>;

TA ta{}; //expected-error{{missing template arguments}}
} // end ns3

namespace ns4 {
template<class T> struct X { 
  template<class U> struct Y { };
};

template<class T> using TA2 = decltype(X<T*>{});
template<class T = int> using TA = TA2<T>;

TA ta{}; //expected-error{{missing template arguments}}

} // end ns4

namespace ns5 {

template<class T> struct X { 
  template<class U> struct Y { };
};

template<class T> using TA2 = X<double *>::Y<T*>;
template<class T = int*> using TA = TA2<T>;

TA ta{}; 
static_assert(is_same<X<double*>::Y<int**>, decltype(ta)>);

} // end ns5

namespace ns6 {

template<class T> struct X { 
  template<class U> struct Y { //expected-note2{{candidate}}
    Y(T, U); //expected-note{{candidate}}
  };
};

template<class T> using TA2 = X<double *>::Y<T*>;
template<class T = int> using TA = TA2<T>;

TA ta{(double*)nullptr, (char *)nullptr}; 
static_assert(is_same<X<double*>::Y<char*>, decltype(ta)>);

TA ta2{(double*)nullptr, 'a'}; //expected-error{{missing template arguments, no viable}}

} // end ns6

namespace ns7_member_template_alias {

template<class T> struct X { 
  template<class U, class V> struct Y { 
    Y(T, U, V);
  };
  template<class T1> using TA = Y<T*, T1>;
};

X<int*>::TA ta{(int*)0, (int**)0, "abc"};

static_assert(is_same<decltype(ta),X<int*>::Y<int**, const char*>>);
namespace ns7_0 {
template<class T, int N> struct Y {
  Y(const T (&)[N]) { }
  template<class U, int O>
  Y(const T(&)[N], const U(&)[O]) { }
};
template<class T, int A1> using TA1 = Y<T, A1>;

TA1 y1{{1, 2, 3}};
IS_SAME(y1, Y<int,3>);
template<class T> using TA2 = Y<T, 3>;
TA2 y2{{'a', 'b', 'c'}};
IS_SAME(y2, Y<char,3>);
template<int N> using TA3 = Y<const char*, N>;
TA3 y3{{"a", "b", "c", "d"}};
IS_SAME(y3, Y<const char*, 4>);
TA3 y33{{"a", "b", "c", "d"},{1, 2, 3}};
IS_SAME(y33, Y<const char*, 4>);
} // end ns7_0
namespace ns7_1 {
template<class T> struct X { 
  template<class U, class V> struct Y { 
    template<class W, int N> Y(T, U, V, const W (&)[N]);
  };
  template<class T1> using TA = Y<T*, T1>;
};

X<int*>::Y y1{(int*)0, (int**)0, "abc", {'a', 'b', 'c'}};
static_assert(is_same<decltype(y1),X<int*>::Y<int**, const char*>>);
X<int*>::TA ta{(int*)0, (int**)0, "abc", {'a', 'b', 'c'}};


} // end ns7_1

namespace ns7_2 {
template<class T0> struct X {
  template<class T, int N> struct Y {
    Y(const T (&)[N]) { }
    template<class U, int O>
    Y(const T(&)[N], const U(&)[O]) { }
  };
  template<class Ta, int Na> using MTA1 = typename X<T0>::template Y<Ta*, Na>;
  template<int Na> using MTA2 = Y<T0, Na>;
}; // X

//X<int>::Y y{{1, 2, 3}, {'3', '4'}};
template<class Ta, int Na> using TA = X<int*>::Y<Ta, Na>;

TA ta{{1, 2, 3}, {'3', '4'}};
IS_SAME(ta, X<int*>::Y<int, 3>);
X<int*>::MTA1 mta1{{"a", "b", "c", "d", "e"}, {1, 2, 3, 4}};
IS_SAME(mta1, X<int*>::Y<const char*, 5>);

X<char*>::MTA2 mta2{{"a"}, {1, 2, 3, 4}}; //expected-warning{{does not allow conversion from string literal}}
IS_SAME(mta2, X<char*>::Y<char*, 1>);

} // end ns7_2
} // end ns7



} // end test_template_alias_deduction

// FIXME: Add tests with partial and explicit specializations etc.
namespace test_partial_spec_and_nested_sfinae {
namespace ns1 {

template<class T0> struct X {
  template<class T> struct Y {
    template<class U1> struct Z {
      typename T0::type mt;
      using Ty = typename T0::type;
      Z(Y<U1>) { }
      Z(Ty) { }
      Z(decltype(Ty{})) { }
    };
    template<class U1> struct Z<U1*> {
      template<class U2> Z(U2) { }
      using partial_type = U1*;
    };
  };
}; 

X<int*>::Y<double>::Z z2{X<int*>::Y<int*>{}}; 
IS_SAME(z2, X<int*>::Y<double>::Z<int*>);
decltype(z2)::partial_type pt1 = (int*) 0;

X<int*>::Y<int>::Z z3{X<int*>::Y<char*>{}}; 
IS_SAME(z3, X<int*>::Y<int>::Z<char*>);
decltype(z3)::partial_type pt2 = (char*) 0;

} //end ns1
namespace ns2 {
template<class T0> struct Y {
  template<class U1> struct Z {
    typename T0::type mt;
    using Ty = typename T0::type;
    Z(U1) { }
    Z(Ty) { }
    Z(decltype(Ty{})) { }
    Z(decltype(mt)) { }
  };
  template<class U1> struct Z<U1*> {
    template<class U2> Z(U2) { }
    using partial_type = U1*;
  };
};

Y<int>::Z<int*> z{(int*)0};
Y<int>::Z z2{(int*)0};  // OK - no errors
} // end ns2

namespace ns3 {
template<class T0> struct Y {
  template<class U1> struct Z {
    Z(U1) { }
    Z(typename T0::type) { } //expected-error{{cannot be used prior to '::'}}
  };
  template<class U1> struct Z<U1*> {
    template<class U2> Z(U2) { }
    using partial_type = U1*;
  };
};

Y<int>::Z<int*> z{(int*)0};
Y<int>::Z z2{(int*)0};  // OK - no errors
Y<int>::Z z3{3};  //expected-note{{in instantiation of}}
} //end ns3
} // end ns test_partial_spec_and_nested_sfinae


namespace injected_class_name {
namespace ns1 {

template<class T> struct A {
  A(T = T());
  int f() {
    A a;  // no deduction.
    A<T>* ap = &a;
    // class template argument deduction
    ns1::A a2{ap};
    // 'A*' is an injected class name
    A<A*>* ap2 = &a2;
    return 0;
  }
}; 
int i = A<int>{}.f();
} // end ns1
namespace ns2 {
template<class T0> struct A { //expected-note2{{candidate}}
  A(T0 = T0{});  //expected-note{{candidate}}
  template<class T> struct B {
    B(T = T{});
    int f() {
      B b;  // no deduction.
      B<T>* bp = &b;
      
      // NOT injected class name for B (although A is injected name below)
      typename A::B b2{bp};
      B<B*>* bp2 = &b2;
      A a;
      A<T0> *ap = &a;
      // deduction
      ns2::A a2{&a};
      //A *ap2 = &a2;
      
      A<T0*> ap3 = 
          A{(T0*)0}; //expected-error{{no matching constructor}}
      A<T0*> ap4 = ns2::A{(T0*)0};
      return 0;
    }
  }; 
};

int I = A<int*>::B<char>{}.f(); //expected-note{{in instantiation of}}
} // end ns2
namespace ns3 {
template<class T> struct A {
  template<class U> struct B;
  template<class U> using ATa = B<U*>; //expected-note{{here}}
  template<class U> struct B {  //expected-note{{here}}
    B(U);
    int f() {
      A::ATa ab{(int*)0};
      typename A<T>::B ab2{ab};
      A<typename A<T>::ATa> *ai; //expected-error{{non-type}}
      A<typename A::B> *ai2;     //expected-error{{non-type}}
      A<decltype(typename A::B{3})> *a3;
    }
    
  };
  template<class U> struct C {
    int g(typename A<T>::C c); //expected-error{{missing template arguments}}
  };
  
}; // end A
auto I = A<int>::B<char*>{0}.f();
} // end ns3
} // end ns injected_class_name

namespace test_simple_template_alias {
namespace ns1 {
template<class T> struct B { B(T); }; //expected-note3{{candidate}}
template<class T> using TB = B<T*>;
TB b{(int*)0};
IS_SAME(b, B<int*>);
TB b2{double{}}; //expected-error{{missing template argument}}

} // end ns1
} //end test_simple_template_alias
namespace variable_templates {
namespace ns1 {

template<class T> struct Y {};

template<class T> Y y{Y<T>()};

Y<int*> y2 = Y{y<int*>};

} // end ns1 

namespace ns2 {
template<class T> struct Y {
  template<class U> Y(T, U);
}; // end ns2

template<class T, class U> Y y = Y{T{}, U{}};

Y y2{Y{y<int *, char*>}};

IS_SAME(y2, Y<int *>);

} // end ns2
namespace ns3 {
template<class T> struct A {
  template<class U1, class U2> struct B { B(U1, U2); };//expected-note3{{candidate}}
};
template<class T> template<class U> A<T>::B<U,U> A<T>::B(U,U); //expected-note{{candidate}}
template<class T> using TA = A<char*>::B<T*, T*>;

//FIXME: This error is triggered by 'v4'
template<template<class ...> class TT, class T = int*, class U = int*>  auto V = TT{(T)0, (U)0}; //expected-error{{could not deduce}} \
                                                                                                 //expected-error{{missing template arguments}}
auto v4 = V<TA,float,float>; // expected-note{{requested here}}
auto v4_0 = V<TA,float,int>; // This is an error, but error appears at the wrong line!

auto v5 = V<A<char*>::B,float, float>; // This is not an error
IS_SAME(v5, A<char*>::B<float, float>);

auto v6 = V<A<char*>::B,float, char*>;
IS_SAME(v6, A<char*>::B<float, char*>);



auto v2 = V<TA>;
IS_SAME(v2, A<char*>::B<int*,int*>);
auto v3 = V<TA,float*,float*>;
IS_SAME(v3, A<char*>::B<float*,float*>);
//IS_SAME(v4, A<char*>::B<float*,float*>);

} //end ns3

} // end ns variable_templates

namespace canonical_deducer_functions {
namespace ns1 {
  template<class T> struct Y;
  template<class T> struct X;
  template<class T> int Y(T); //expected-error{{must be a template id}}
  template<class T> X<T> Y(T); //expected-error{{must be a template id referring}}
  template<class T> struct Y;
  template<class T> Y<T> Y(T); 
  template<class T> Y<T*> Y(T); 
  template<class T> auto Y(T*) -> Y<T>;
} // end ns1  
namespace ns2 {
  template<class T> struct A {
    template<class U> struct B;
    template<class U> B<U> B(U) const; //expected-error{{class template deducer}}
    template<class U> B<U> B(U) volatile; //expected-error{{class template deducer}}
    template<class U> B<U> B(U) &; //expected-error{{class template deducer}}
    template<class U> B<U> B(U) &&; //expected-error{{class template deducer}}
    template<class U> constexpr B<U> B(U); //expected-error{{class template deducer}}
    template<class U> friend A<U> A(U);  //expected-error{{friend}}
    template<class U> inline B<U> B(U); // expected-error{{inline}}
    template<class U> static B<U> B(U); //expected-error{{static}}
   };
   template<class U> inline A<U> A(U); // expected-error{{inline}}
   template<class U> static A<U> A(U); //expected-error{{static}}
   template<class U> extern A<U> A(U); //expected-error{{extern}}
} // end ns2

namespace ns3 {
template<class T> struct A {
  template<class U> struct B { B(U); B(char*); };
  template<class U> B<U> B(U);
  template<class U> B<U> B(U*);
};
A<int>::B b{(char*)0};
IS_SAME(b, A<int>::B<char>);

} // end ns3
namespace ns4 {
template<class T> struct A {
  template<class U> struct B;
};

template<class T> template<class U> A<T>::B<U> A<T>::B(U) = delete; //expected-note{{candidate}}

A<int>::B b{(char*)0}; //expected-error{{deleted}}

} // end ns4
namespace ns5 {
template<class T> struct A {
  template<class U> struct B
  { B(U); } //expected-note{{candidate}}
  ;
};

template<class T> template<class U> A<T>::B<U> A<T>::B(U) = delete; //expected-note{{candidate}}

A<int>::B b{(char*)0}; //expected-error{{ambiguous}}

} // end ns5

namespace ns6 {
template<class T> struct A {
  template<class U> struct B 
  { B(U);};
  
  template<class U> B<U> B(U); //expected-note{{previous}}
};
template<class T> template<class U> A<T>::B<U> A<T>::B(U) = delete; //expected-error{{must be first}}

} // end ns6
namespace ns7 {
template<class T> struct A {
  template<class U> struct B;
  template<class U> B<U> B(U*) { } 
#ifndef DELAYED_TEMPLATE_PARSING
  //expected-error@-2{{definition}}
#endif
};
template<class T> template<class U> A<T>::B<U> A<T>::B(U) { }
#ifndef DELAYED_TEMPLATE_PARSING
  //expected-error@-2{{definition}}
#endif

} // end ns7 
} // end ns canonical_deducer_functions

namespace test_more_template_aliases {
namespace ns1 {
template<class T, class U, int N, int M> struct X { X(const T (&)[N], const U (&)[M]); }; //expected-note{{in initialization}} expected-note{{argument}} \
                                                                                          //expected-note3{{candidate}}
template<class T, int N> using TA = X<T, T, N, N+1>;
TA ta{{1, 2, 3}, {1, 2, 3, 4}};
TA ta2{{1, 2, 3}, {1, 2, 3, 4, 5}}; //expected-error{{excess}}

TA ta3{{1, 2, 3}, {"1", "2", "3", "4"}}; //expected-error{{missing template arguments}}

} // end ns1
namespace ns2 {
template<class T, T N> struct X {
  X(T);
};
template<class T, int N> struct X<T*, N> {
  X(T*, const int (&)[N]);
}; 

template<class T> using TA = X<T*,5>; //expected-error{{not implicitly convertible}}
TA ta{(int*)0};//expected-note{{instantiation}}
}// end ns2

namespace ns3 {
template<class T, int N> struct X {
  X(T);
};
template<class T, int N> struct X<T*, N> { //expected-note2{{candidate}}
  X(T*, const int (&)[N]); //expected-note{{candidate}}
}; 

template<class T> using TA = X<T*,5>; 
TA ta{(int*)0};//expected-error{{no matching constructor}}
}// end ns3

} // end test_more_template_aliases


namespace test_more_explicit_arguments867 {
namespace ns1 {
template<class T, class U> struct A {
  A(U);
  A(...);
};
A<int> a('a');
IS_SAME(a, A<int, char>); 
template<class T, class U> A<T,U> A(T, U);
A<int> a2('a', (char*)0);
IS_SAME(a2, A<int, char*>);
template<class T, class ... Rest> struct first { using type = T; };
template<class T, class U, class ... Rest> struct second { using type = U; };

template<class ... Ts> A<typename first<Ts...>::type, typename second<Ts...>::type> A(Ts...);
A<> a3{1, 'a', 3.14};  
IS_SAME(a3, A<int, char>);
} // end ns1
namespace ns2 {
template<class T, class U> struct A {
  A(U);
  A(...);
};
template<class T, class ... Rest> struct first { using type = T; };
template<class T, class U, class ... Rest> struct second { using type = U; };

template<class ... Ts> A<typename first<Ts...>::type, typename second<Ts...>::type> A(Ts...);
A<> a{1, 'a', 3.14};  
IS_SAME(a, A<int, char>);
A<int*> a2(1, 'a', 3.14);//expected-error{{not consistent with explicit}}
A<int> a3{'a', "abc", 3.14}; //expected-error{{not consistent with explicit}}

} // end ns2

namespace ns3 {
template<class T, class U> struct A {  //expected-note{{template is declared here}}
  A(U); 
  A(...);  
};

template<class ... Ts> A<Ts...> A(Ts...); 
A<int> a3{'a', "abc"}; 
IS_SAME(a3, A<int, const char*>);
A<int*> a2(1, 'a');//expected-error{{template arguments}}
} // end ns3

namespace ns4 {

template<class T, class U> struct A { A(...) { } };

template<class T, class U> A<U,T> A(T, U);

A<int> a("abc", 'a');
IS_SAME(a, A<int, const char*>);
} // end ns4

namespace ns5 {
template<class T = int> struct X { X(T); };
X<> x{'a'}; 
IS_SAME(x, X<char>);
} //end ns5

} // end ns test_more_explicit_arguments867
namespace test_nested_member_alias_template_references {
namespace ns1 {
template<bool B>
struct bool_constant { };
template<> struct bool_constant<true> { using type = int*; };
template<typename... A>
struct F
{
  template<typename... B>
    using SameSize = typename bool_constant<sizeof...(A) == sizeof...(B)>::type;

  template<typename... B, typename = SameSize<B...>>
  F(B...) { }
  F(...) { } 
};

template<typename... A>
struct F2
{
  template<typename... B>
    using SameSize = typename bool_constant<sizeof...(A) == sizeof...(B)>::type;

  template<typename... B, typename = SameSize<B...>>
  F2(F<A...>,F<B...>) { }
  //F2(...) { } 
};


void func()
{
  F<int,char> f1(3);
  F<double, float> f2(3);
  F2 ff2{f1, f2};
  F2<int,char> *ff2p = &ff2;
}

template<class T0, class T1> struct X { };
template<class T> struct A {
  template<class U> using TA = X<T, U>;
  template<class V>
  A(TA<V>) { }
};
A a{X<int, int*>{}};
int main() {
  func();
  return 0;
}
} // end ns1
namespace ns2 {

template<class T0, class T1, class T2> struct X { };
template<class T> struct A {
  template<class U, class U2> using TA2 = X<U,U2*,T**>;
  template<class U> using TA = TA2<U,T*>;
  template<class V>
  A(TA<V>) { }
};
A a{X<int, int**,int**>{}};

} // end ns2
namespace ns3 {
template<class T0, class T1, class T2> struct X { };
template<class ... T> struct A {
  template<class U, class U2, class U3> using TA2 = X<U, U2, U3>;
  template<class U> using TA = X<U,T...>;
  template<class V>
  A(TA<V>) { }
};
A a{X<float, int*,char*>{}};
A<int*,char*> *ap = &a;

} // end ns3
} // end test_nested_member_alias_template_references

namespace test_nested_member_types1056 {

namespace ns1 {
template<class T, class U> struct X { };
template<class T> struct A {
  struct B { using type = X<T, T*>; };
  template<class V>
  A(T, V, typename B::type) { }
};
A a{(char*)0, (int*)0, X<char*, char**>{}};
} //end ns1
namespace ns2 {
template<class T, class U> struct X { };
template<class T> struct A { //expected-note 2{{candidate}} implicit ctors
  struct B { using type = X<T, T*>; };
  template<class V>
  A(T, V, typename B::type) { } //expected-note{{no known conversion from 'X<[...], char *>' to 'X<[...], char **>' for 3rd argument}}
};
A a{(char*)0, (int*)0, X<char*, char*>{}}; //expected-error{{missing template arguments}}
} //end ns2
namespace ns3 {
template<class T, class U> struct X { };
template<class T> struct A { 
  struct B { struct C { using type = X<T, T*>; }; };
  template<class V>
  A(T, V, typename B::C::type) { } 
};
A a{(char*)0, (int*)0, X<char*, char **>{}}; 

} //end ns3

namespace ns4 {
template<class T, class U, class V> struct X { };
template<class T> struct A {
  template<class U> struct B { template<class V> struct C { using type = X<T,U,V>; }; };
  template<class U>
  A(T, U, typename B<U>::template C<T>::type) { }
};
A a{(char*)0, (int*)0, X<char*, int*, char*>{}};

} // end ns4
namespace ns5 {
template<class T, class U> struct X { };
template<class T> struct A {
  template<class U> struct B { using type = X<T, U>; };
  template<class V>
  A(T, V, typename B<V>::type) { }
};
A a{(char*)0, (int*)0, X<char*, int*>{}};

} //end ns5
namespace ns6 {
template<class T, class U> struct X { };
template<class T> struct A {
  template<class U> struct B { using type = X<T, U>; };
  template<class V, class W = typename B<V>::type>
  A(T, V) { }
};
A a{(char*)0, (int*)0};
namespace ns6_1 {
template<class T> struct A { //expected-note 2{{candidate}} implicit ctors
  template<class U> struct B { };
  template<class V, class W = typename B<V>::type>
  A(T, V) { } //expected-note{{candidate}}
};
A a{(char*)0, (int*)0}; //expected-error{{missing template arguments}}

} // end ns6_1
} // end ns6
namespace ns7 {

template<class T> struct A {
  static auto foo() { return T{}; }
  template<class V, class W = decltype(foo())>
  A(T, V) { }
};
A a{(char*)0, (int*)0};

namespace ns7_1 {
template<class T> struct A {
  static auto foo() { return (T*)0; }
  A(...);
};
template<class T> A<T> A(T, decltype(A<T>::foo()));

A a{(char*)0, (char**)0};
} // end ns7_1
namespace ns7_2 {
template<class T> struct A {
  static auto foo() { return (T*)0; }
  template<class V, class W = decltype(foo())>
  A(T, V, decltype(foo())) { }
};
A a{(char*)0, (int*)0, (char**)0};

} // end ns7_2
namespace ns7_3 {
template<class T, class U> struct X { };
template<class T> struct A {
  template<class U>
  static auto foo(U) { return (X<T,U>*)0; }
  template<class V, class W = decltype(foo(V{}))>
  A(T, V, decltype(foo(V{}))) { }
};
A a{(char*)0, (int*)0, (X<char*,int*>*)0};

} // end ns7_3
namespace ns7_4 {
template<class T, class U> struct X { };
template<class T> struct A {
  template<class U, class V = T>
  static auto foo(U) { return (X<T,U>*)0; }
  template<class V, class W = decltype(foo(V{}))*>
  A(T, V, decltype(foo(V{}))) { }
};
A a{(char*)0, (int*)0, (X<char*,int*>*)0};

} // end ns7_4
namespace ns7_5 {

template<class T> struct A {
  template<class V = int*>
  A(T, V = (V)(T)nullptr) { }
};
A a{(char*)0};

} // end ns7_5
namespace ns7_6 {

template<class T> struct A {
  template<class V = int*>
  A(T, V = (T)nullptr) { } //expected-error{{cannot initialize}} expected-note{{passing argument}}
};
A a{(char*)0}; //expected-note{{in instantiation of}}

} // end ns7_6

namespace ns7_7 {

template<class T> struct A {
  template<class V = int*>
  A(T, V = (T)nullptr) { } 
  A(char*);
};
A a{(char*)0}; 

} // end ns7_7

} // end ns7

} // end ns test_nested_member_types1056

namespace more_tests_1032 {
namespace ns1 {
template<class T, class U = char*> struct A { A(T); };
template<class T> using TA = A<T>;

TA ta{3};
A<int, char*> *tap = &ta;

} // end ns1
namespace ns2 {
template<class T = int>
struct A { };
A<> a;
A a2;
} // end ns2
namespace ns3 {
template<template<class T> class TT, class T> auto var = TT<>{T{}};
template<class T = char*> struct X { template<class U> X(U) { } };

auto v = var<X,int*>;
namespace ns3_1 {
template<template<class T> class TT, class T> auto var = TT{T{}};
template<class T = char*> struct X { template<class U> X(U) { } };

auto v = var<X,int*>;

} //end ns3_1

} //end ns3
namespace ns4 {

int *IP;
float *FP;

template<class T>
struct A {
  template<class U, class V> struct B { B(U,V) { } };
  template<class U>
  A(T, U, B<T,U>) { }
};
A a2{IP, FP, A<int*>::B{IP, FP}};
A<int*> *ap2 = &a2;

//A a3{A<int*>::B{(char*)0, (float*)0}};
//A<char *> *ap3 = &a3;


} // end ns4
namespace ns5 {
template<class T>
struct A {
  template<class U, class V> struct B { B(U,V) { } };
  template<class U>
  A(A<T>::B<T,U>) { }
};
// FIXME: This should not work!
A a2{A<int*>::B{(int*)0, (float*)0}};
A<int*> *ap2 = &a2;


//A a3{A<int*>::B{(char*)0, (float*)0}};
//A<char *> *ap3 = &a3;


} // end ns5
namespace ns6 {
template<class T>
struct A {
  template<class U, class V> struct B{
    template<class U1, class V1> struct C { };
  };
  template<class U>
  A(T, U, typename A<T>::B<T,U>::template C<T*,U*>) { }
  //A(A::B<T,U>) { }
  //A(typename A<T>::template B<T,U>) { }
};
A a2{(int*)0, (float *)0, A<int*>::B<int*, float*>::C<int**,float**>{}};

} // end ns6
namespace ns7 {
template<class T>
struct A {
  template<class U, class V> struct B{
    template<class U1, class V1> struct C { };
  };
  //template<class U>  A(T, U, typename A<T>::B<T,U>::template C<T*,U*>) { }
  //template<class U>  A(T, U, A::B<T,U>) { }
  //template<class U>  A(T, U, typename A<T>::template B<T,U>) { }
  template<class U> A(T, U, B<T,U>);
};

A a2{(int*)0, (float*)0, A<int*>::B<int*,float*>{}};
} // end ns7
namespace ns8 {
template<class T>
struct A {
  struct B{
    struct C { struct D { }; };
  };
  template<class U> A(T, U, typename B::C::D);
};

A a2{(int*)0, (float*)0, A<int*>::B::C::D{}};
} // end ns8

namespace ns9 {
template<class T>
struct A {
  struct B{
    template<class U, class V>
    struct C { struct D { }; };
  };
  template<class U> A(T, U, typename B::template C<T,U>::D);
};

A a2{(int*)0, (float*)0, A<int*>::B::C<int*, float*>::D{}};

} // end ns9

namespace ns10 {

template<class T> struct Id { using type = T; };
template<class U> struct X {  };
template<class T> struct A {
  
  static auto foo() { return X<T>{}; }
  template<class V, class W = decltype(foo())>
  A(T, V, typename Id<W>::type) { }
};
A a{(char*)0, (int*)0, X<char*>{}};
} // end ns10
namespace ns11 {

template<class T> struct Id { using type = T; };

template<class T> struct A { //expected-note 2{{candidate}}
  template<class U> struct X {  };  
  static auto foo() { return X<T>{}; }
  template<class V, class W = decltype(foo())>
  A(T, V, typename Id<W>::type) { } //expected-note{{candidate}}
};
A a{(char*)0, (int*)0, A<char*>::X<char*>{}};
A a2{(char*)0, (int*)0, A<int*>::X<char*>{}}; //expected-error{{missing template arguments}}

} // end ns11
namespace ns12 {

template<class T> struct Id { using type = T; };

template<class T> struct A { 
  template<class U> struct X {  };  
  static auto foo() { return X<T>{}; }
  template<class U> void foo(U) { }
  template<class V, class W = decltype(foo())>
  A(T, V, typename Id<W>::type) { } 
};
A a{(char*)0, (int*)0, A<char*>::X<char*>{}};

} // end ns12
namespace ns13 {

template<class T> struct Id { using type = T; };

template<class T> struct A { 
  template<class U> struct X {  };  
  static auto foo() { return X<T>{}; }
  template<class U> static auto foo(U) { return X<U>{}; }
  template<class V, class W = decltype(foo(V{}))>
  A(T, V, typename Id<W>::type) { } 
};
A a{(char*)0, (int*)0, A<char*>::X<int*>{}};

} // end ns13
namespace ns14 {
template<class T> struct X { X() {} };
template<template<class T = int*> class TT> auto foo() {
  TT<> tt;
  return tt;
}
template<class T = char*> struct Y { Y() { } };

void foo() {
  decltype(foo<X>())* p = (X<int*>*)0;
  decltype(foo<Y>()) *p2 = (Y<char*>*)0;
}
} // end ns14
} // end more_tests_1032

