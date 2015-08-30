// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING


namespace test_all_literal_types_except_void {

namespace ns0 {
  template<void V> struct X; //expected-error{{cannot have type 'void'}}
  template<void V> void f(); //expected-error{{cannot have type 'void'}}
} //end ns0

namespace ns1 {
  template<char Str[5]> struct X;
} // end ns1

} // end test_all_literal_types_except_void


namespace test_literal_class_specializations {
namespace ns0 {
// Test if we can type literal classes directly as template arguments.
struct L { int I; char C; };

template<L A> struct S { };
template<> struct S<L{5, 'a'}> { }; //expected-note3{{previous definition}}
constexpr L l5a = L{5, 'a'};
template<> struct S<l5a> { };  //expected-error{{redefinition}}
template<> struct S<L{l5a}> { };  //expected-error{{redefinition}}
template<> struct S<L{l5a.I, l5a.C}> { };  //expected-error{{redefinition}}
template<> struct S<L{l5a.I + 1, l5a.C}> {};

} // end ns0

namespace ns0_check_nullptr_specializations {

struct X { int *Ip; };

template<X A> struct S { };
template<> struct S<X{nullptr}> { }; //expected-note{{previous definition}}
template<> struct S<X{nullptr}> { }; //expected-error{{redefinition}}

} // end ns0_check_nullptr_specializations

namespace ns0_check_ref_ptr_specializations {
namespace ns0 {
struct X { int *Ip; };

template<X A> struct S { };
extern int GlobalI;
int GlobalI = 10;
int &GlobalRef = GlobalI;
template<> struct S<X{&GlobalRef}> { };//expected-note{{previous}}
template<> struct S<X{&GlobalI}> { };  //expected-error{{redefinition}}
} //end ns0

namespace ns1 {
struct X { int *Ip; };

template<X A> struct S { };
extern int GlobalI;
int GlobalI = 10;
int &GlobalRef = GlobalI;
constexpr X GlobalXVar{&GlobalI};
template<> struct S<X{GlobalXVar.Ip}> { }; //expected-note{{previous}}
template<> struct S<X{&GlobalRef}> { }; //expected-error{{redefinition}}
} //end ns1

namespace ns2 {
struct X { int &Ip; };

template<X A> struct S { };
extern int GlobalI;
int GlobalI = 10;
int &GlobalRef = GlobalI;
template<> struct S<X{GlobalRef}> { }; //expected-note{{previous}}
template<> struct S<X{GlobalI}> { }; //expected-error{{redefinition}}
} //end ns2



} // end ns0_check_ref_ptr_specializations
namespace test_base_class_iteral_specs {

struct X { int *Ip; };
struct Y : X { char C; constexpr Y(int &I, char C) : X{&I}, C{C} { } };

template<Y A> struct S { };
extern int GlobalI;
int GlobalI = 10;
int &GlobalRef = GlobalI;

template<> struct S<Y{GlobalRef, 'A'}> { }; //expected-note{{previous}}
template<> struct S<Y{GlobalI, 'A'}> { }; //expected-error{{redefinition}}

} // end ns test_base_class_iteral_specs
namespace ns1 {
struct X { int I; };

template<X A> struct S { };

constexpr X x{2};
template<> struct S<x> { typedef int type; }; //expected-note{{declared here}}


void test() {
  constexpr X y{2};
  S<x>::type I = 5;
  S<y>::type I2 = 5;
  // z does not contain the same value as x or y - so should not specialize to the same struct.
  constexpr X z{3};
  S<z>::type I3 = 5; //expected-error{{no type named 'type'}}
}
} // end ns1

} // end test_literal_class_specializations

namespace test_string_literals_ban {
namespace ns1 {
template<char [5]> struct X { using type = int*; };
template<> struct X<"abc"> { using type = float*; };
template<> struct X<"1234"> { using type = float**; };
template<> struct X<"12345"> { using type = float***; }; //expected-error{{too long}}
X<"abc">::type I = (float*)0;
X<{'a', 'b', 'c'}>::type I2 = (float*)0;

X<"abcd">::type I3 = (int*)0;
X<{'a', 'b', 'c', 'd'}>::type I4 = (int*)0;
} // end ns1

namespace ns2 {

char C = 'a';
constexpr const char *const str = "abc";
constexpr const char *const str2 = str;
template<const char *> struct XPtr;
template<> struct XPtr<str> { }; //expected-error{{address of string literal}}
template<> struct XPtr<{str}> { }; //expected-error{{address of string literal}}
template<> struct XPtr<{str, &C}> { }; //expected-error{{excess elements}}
template<> struct XPtr<(str, &C)> { }; // this should be ok

const char arr_ok[] = "abc";
template<> struct XPtr<arr_ok> { };   // this should be ok
namespace ns2_test_one_past_end {
// Test forbid one past end
const char arr_ok[] = "abc";
template<const char *> struct XPtr;
template<> struct XPtr<arr_ok> { };   //expected-note{{previous}}

template<> struct XPtr<arr_ok + 0> { }; //expected-error{{redefinition}}  
template<> struct XPtr<arr_ok + 1> { };   
template<> struct XPtr<arr_ok + 2> { };   
template<> struct XPtr<arr_ok + 3> { };   
template<> struct XPtr<arr_ok + 4> { }; //expected-error{{one past end}}
char C = 'a';
constexpr char *pC = &C;
template<> struct XPtr<&C> { };   //expected-note{{previous}}

template<> struct XPtr<pC> { }; //expected-error{{redefinition}}  
template<> struct XPtr<pC + 1> { }; //expected-error{{one past end}}
template<> struct XPtr<pC + 2> { }; //expected-error{{not a constant expression}} expected-note{{cannot refer to element}}

namespace ns2_1 {

const char arr_ok[][4] = { "abc" };
template<const char *> struct XPtr;
template<> struct XPtr<arr_ok[0]> { };
template<> struct XPtr<arr_ok[0] + 1> { };

} // end ns2_1  
} // end ns2_test_one_past_end

namespace ns2_2 {

const char arr_ok[][4] = { "abc" };
template<const char (*)[4]> struct XPtr;
template<> struct XPtr<arr_ok> { }; //expected-note{{previous}}
template<> struct XPtr<&arr_ok[0]> { }; //expected-error{{redefinition}}
template<> struct XPtr<&"abc"> { }; //expected-error{{takes address of string literal}}
struct B {
  const char (*mb)[4];
};

template<B b> struct X { };
template<> struct X<B{&"abc"}> { }; //expected-error{{takes address of string literal}}

} // end ns2_2
} // end ns2

namespace ns3 {

constexpr const char (&str)[5] = "abcd";
constexpr const char (&str2)[5] = str;

template<const char (&)[5]> struct XRef;
template<> struct XRef<"abcd"> { };  //expected-error{{address of string literal}}
template<> struct XRef<str2> { };  //expected-error{{address of string literal}}

template<const char &> struct XRef2;
template<> struct XRef2<*str> { }; //expected-error{{address of string literal}}

} //end ns3

namespace ns4_dont_allow_ptrs_or_refs_to_strings_in_subobjects {
struct S { const char *Lit; }; 
template<S s> struct XStruct { };
template<> struct XStruct<{nullptr}> { };
template<> struct XStruct<{"abc"}> { }; //expected-error{{address of string literal}}

namespace ns4_1 {
struct S { const char (&Lit)[4]; }; 
template<S s> struct XStruct { };
template<> struct XStruct<{"abc"}> { }; //expected-error{{address of string literal}}
} // end ns4_1
} // end ns4_dont_allow_ptrs_or_refs_to_strings_in_subobjects


namespace ns4_check_in_unions_and_variants {

union U {
 const char *c;
 int I;
};

template<U u> struct X { };
template<> struct X<U{"abc"}> { }; //expected-error{{address of string literal}}


struct V {
  union {
    const char * c;
    int I;
  };
};

template<V v> struct X2 { };
template<> struct X2<V{"abcd"}> { }; //expected-error{{address of string literal}}


} // end ns4_check_in_unions_and_variants
} // end test_string_literals_ban

namespace test_array_vars_and_values {
namespace ns1_must_be_const {

char arr_ok[4] = "abc";
                       
template<char [4]> struct XPtr;
template<> struct XPtr<arr_ok> { }; //expected-error{{not a constant expression}}


} // end ns1_must_be_const
namespace ns1_array_vars_ok {
const char arr_ok[4] = "abc";
                       
template<char [4]> struct XPtr;
template<> struct XPtr<arr_ok> { }; //expected-note{{previous}}
template<> struct XPtr<{'a', 'b', 'c', 0}> { }; //expected-error{{redefinition}}

} // end ns1_array_vars_ok


} // end ns test_array_vars_and_values

namespace test_literal_subobjetcs {
namespace ns1 {
struct S { char arr[5]; int I; };

constexpr S SObj{"abcd", 5};
template<const int *P> struct XPtr;
template<> struct XPtr<&SObj.I> { };
template<> struct XPtr<&SObj.I + 1> { }; //expected-error{{must not have one past end}}

template<const int &P> struct XRef;
template<> struct XRef<SObj.I> { };

template<S s> struct XS0;
template<> struct XS0<SObj> { };

template<S s> struct XS01;
template<> struct XS01<SObj> { }; //expected-note{{previous}}
template<> struct XS01<{{'a', 'b', 'c', 'd'}, 5}> { }; //expected-error{{redefinition}}

template<S s> struct XS;
template<> struct XS<SObj> { }; //expected-note{{previous}}
template<> struct XS<{"abcd", 5}> { }; //expected-error{{redefinition}}

} // end ns1
} //end ns test_literal_subobjetcs

namespace test_fptr {
  int X(int) { return 0; }
template<int (*X)(int)> decltype(X) foo() {
  return X;
};


int main() {
  foo<&X>();
  return 0;
}

}// end test_fptr

namespace test_unions {
  namespace ns1 {
    union U {
      int I1 = 0;
      int I2;
    } U;
    
    template<int &I> struct X;
    template<> struct X<U.I1> { }; //expected-error{{union member}}
  } // end ns1

  namespace ns2 {
    static union {
      int I1 = 0;
      int I2;
    };
    template<int *> struct X;
    template<> struct X<&I1> { }; //expected-error{{union member}}
  } // end ns2
  namespace ns3 {
    constexpr struct S {
      union U {
        int I1 = 0;
        int I2;
      } u;
      int I3 = 3;
    } s{};
    template<const int *> struct X;
    template<> struct X<&s.I3> { };
    template<> struct X<&s.u.I1> { }; //expected-error{{union member}}
    template<int N> int* f(); //expected-note{{candidate}}
    template<const int &R> char *f(); //expected-note{{candidate}}
    template<const int *P> int** f(); 
    int GI;
    int **ip = f<&s.I3>();
    int **ip2 = f<&GI>();
    // This can not bind to a reference
    int *ip3 = f<s.u.I1>();
    
    int *ip4 = f<s.I3>(); //expected-error{{ambiguous}}
  } // end ns3
} // end test_
namespace test_ok_to_use_dereferenced_string_literals {

template<char C> struct X;
template<> struct X<*"abc"> { }; //expected-note2 {{previous}}
template<> struct X<'a'> { }; //expected-error {{redefinition}}
template<> struct X<(&"abc")[0][0]> {}; //expected-error{{redefinition}}
} //test_ok_to_use_dereferenced_string_literals

namespace test_literals_in_funs {


} // ens ns test_literals_in_funs

namespace test_lvalues_as_nttps {
namespace ns1 {
struct A {
  int a = 0;
  int arr[3] = { 11, 22, 33 };
};

struct X {
  struct Y {
    int yI = 5;
    //int yArr[5] = { 1, 2, 3, 4, 5 };
    A yArr[5] = { A{1}, A{2}, A{3}, A{4} };
    //char yArr[5] = "abcd";
    
  } y{};
} x{};

template<int *I> int& f() { return *I; };

template<> int& f<x.y.yArr[2].arr + 1>() { static int I; return I; } //expected-note{{previous}}
template<> int& f<x.y.yArr[2].arr + 1>() { static int I; return I; } //expected-error{{redefinition}}

template<> int& f<x.y.yArr[2].arr + 2>() { static int I; return I; } //expected-note{{previous}}
constexpr int *get_addr() { return (x.y.yArr + 2)->arr + 2; }
template<> int& f<get_addr()>() { static int I; return I; } //expected-error{{redefinition}}

template<int *I> constexpr int* fc() { return I; };

void test() {
  static_assert(fc<x.y.yArr->arr + 1>() == x.y.yArr[0].arr + 1, "");
}
} // end ns1

namespace ns2 {
struct A {
  int a = 0;
  char arr[5] = "abcd";
};

struct X {
  struct Y {
    int yI = 5;
    A yArr[5] = { A{1}, A{2}, A{3}, A{4} };    
  } y{};
} x{};

template<char *I> char& f() { return *I; };

template<> char& f<x.y.yArr[2].arr + 0>() { static char I; return I; } //expected-note{{previous}}
template<> char& f<x.y.yArr[2].arr>() { static char I; return I; } //expected-error{{redefinition}}

template<> char& f<x.y.yArr[2].arr + 3>() { static char I; return I; } //expected-note{{previous}}
constexpr char *get_addr() { return (x.y.yArr + 2)->arr + 3; }
template<> char& f<get_addr()>() { static char I; return I; } //expected-error{{redefinition}}

template<char *I> constexpr char* fc() { return I; };

void test() {
  static_assert(fc<x.y.yArr->arr + 1>() == x.y.yArr[0].arr + 1, "");
}
} // end ns2

namespace ns3_test_base {
struct B { int b; } gB{5};

struct D : B {
  float d;
  D(float d, int b) : d(d), B{b} { }
} gD{3.14, 10};

template<B &tb> int f() { return tb.b; }

void main() {
  
  int I = f<gD>();
  int J = f<gB>();
  
}  

} // end ns3_test_base

} // end test_lvalues_as_nttps
namespace test_passing_nttps_thru {
namespace ns1 {
struct B {
  double b = 3.14;
};
struct D : B {
  double d = 6.27;
};

template<B b> B f() { return b; }
template<B bg> B g() { return f<bg>(); }

int main() {
  constexpr B bl{};
  B b = g<bl>();
  B b2 = g<D{}>();
}
} // end ns1

} // end ns test_passing_nttps_thru

namespace test_template_argument_deduction {

namespace ns0 {
  template<typename T>
  struct it_is_a_trap { 
    typedef typename T::trap type;
  };
  template<> struct it_is_a_trap<char> {
    using type = char;
  };
  
  using B = bool;
  template<B, typename T = int*>
  struct enable_if {
    typedef T type;
  };

  template<typename T>
  struct enable_if<{false}, T> { };

  template<typename T>
  typename enable_if<{sizeof(T) == 1}>::type 
  f(const T&, typename it_is_a_trap<T>::type* = 0);

  float* f(...);

  void test_f() {
    int *ip = f('a');
    float *fp = f(5);
  }
} // end ns0


namespace ns1 {
  template<typename T>
  struct it_is_a_trap { 
    typedef typename T::trap type;
  };
  template<> struct it_is_a_trap<char> {
    using type = char;
  };
  
  struct B { bool b; };
  template<B, typename T = int*>
  struct enable_if {
    typedef T type;
  };

  template<typename T>
  struct enable_if<{false}, T> { };

  template<typename T>
  typename enable_if<{sizeof(T) == 1}>::type 
  f(const T&, typename it_is_a_trap<T>::type* = 0);

  float* f(...);

  void test_f() {
    int *ip = f('a');
    float *fp = f(5);
  }
} // end ns1

} //end ns test_template_argument_deduction
namespace test_variadic_nttps {

namespace ns1 {
struct B {
  char mb[5] = "abcd";
};

template<B b> struct X { enum { value = b.mb[0] }; };

template<B ... bs> constexpr int f(X<bs> ... xs) {
  return sizeof...(xs)  
      + (xs.value + ...)
      + (bs.mb[0] + ...)
      ;
}

int I = f(X<B{"abc"}>{}, X<B{"def"}>{});


} // end ns1 

namespace ns2 {
struct B {
  char mb[5] = "abcd";
};

template<B b, int I> struct X { enum { value = b.mb[I] }; };

template<B ... bs, int ... I> constexpr int f(X<bs, I> ... xs) {
  return (bs.mb[I] + ...) 
     + (xs.value + ...)
      ;
}

static_assert(f(X<B{"abc"},0>{}, X<B{"def"}, 1>{}) == ('a' + 'e') * 2, "");

} // end ns2
} // end ns test_variadic_nttps

namespace test_not_modifiable {
namespace ns1 {

struct B {
  float f;
};

template<B b> void f2() {
  b.f = 2.2; 
#ifndef DELAYED_TEMPLATE_PARSING  
  //expected-error@-2{{not assignable}}
#endif

}

} //end ns1

} // end test_not_modifiable
namespace test_mem_funs_on_nttps {

namespace ns1 {
struct B {
  float f;
  constexpr float mf() const { return f; }
};


template<B b> constexpr float g() {
  return b.mf();
}

constexpr float F = g<{3.14}>();
constexpr float G = 3.14;
static_assert(F == 3.14F, "");

} //end ns1

namespace ns2 {
struct B {
  float f;
  constexpr float mf() { return f; } //expected-note{{declared here}}
};


template<B b> constexpr float g() {
  return b.mf(); //expected-error{{not viable}}
}

constexpr float F = g<{3.14}>(); //expected-error{{must be initialized by a constant expression}}\
                                 //expected-note{{in instantiation}}


} //end ns2


}
namespace test_nttps_returned_by_mem_funs {
namespace ns1 {

union U {
  int J = 5;
  int I;
  constexpr U() { }
  constexpr U(int I) : I(I) { }
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

template<U u> struct Y {
  int foo() {
    constexpr int J = u.J;
    u.I = 4; //expected-error{{expression is not assignable}}
    return J;
  }
};
template<> struct Y<U(3)> {
  int goo() {
    constexpr int I = 3;
    return I;
  }
};


int main() {
  X<{}> xu;
  xu.foo();
  xu.foo2();
  Y<U{}>{}.foo();
  Y<U{3}>{}.goo();
  return 0;
}

} // end ns1
} // end ns test_nttps_returned_by_mem_funs

namespace test_transform_arrays_initialized_by_string_literals {
namespace ns1 {

template<int N> constexpr int f(const char (&str)[N]) {
  constexpr int I = N;
  return N + I;
}

template<char CStr[10]> constexpr char foo() {
  const char *P = CStr;
  static_assert(f(CStr) == 20, "");
  return CStr[f(CStr) - 20];
}

static_assert(foo<"#$%">() == '#', "");

} // end ns1
} //end ns test_transform_arrays_initialized_by_string_literals

namespace test_member_pointers {
namespace ns1 {
struct A {
  int N;
} a;

template<int &N> struct X { };

template<> struct X<a.*&A::N> { }; //expected-note{{previous}}
template<> struct X<a.N> { }; //expected-error{{redefinition}}

} // end ns1

namespace ns2 {
struct B { char bc; };
struct D : B { };

template<char D::* Mbc> void foo() {
  
}
template<> void foo<&B::bc>(){ }

char D::* GBC = &B::bc;

template<char D::*&Mbc> void foo() { }
template<> void foo<GBC>() { }

} // end ns2

namespace ns3 {

struct B { char bc; };
struct D : B { constexpr D(char c) : B{c} { } };

template<char D::* Mbc, D d> constexpr char foo() {
  return d.*Mbc;
}
static_assert(foo<&B::bc, D{'#'}>() == '#', "");
static_assert(foo<&D::bc, D{'#'}>() == '#', "");

template<char D::* Mbc> void goo() { }
template<> void goo<&B::bc>() { } //expected-note{{previous}}
template<> void goo<&D::bc>() { } //expected-error{{redefinition}}

} // end ns3

namespace ns4 {

struct B { char bc; };
struct D : B { constexpr D(char c) : B{c} { } };

struct A {
  int N;
  char B::* mbc;
};

template<A a, B b> constexpr char f() { 
  return b.*a.mbc;
}

static_assert(f<{3, &B::bc}, B{'%'}>() == '%', "");  
static_assert(f<{3, &D::bc}, B{'$'}>() == '$', "");  
template<> constexpr char f<{3, &B::bc}, B{'c'}>() { return '*'; }
static_assert(f<{3, &D::bc}, B{'c'}>() == '*', "");  

} // end ns4
} // end test_member_pointers

namespace test_unimplemented {
namespace ns1_test_extension_vectors {

typedef int v4si __attribute__((__vector_size__(16)));
template<v4si vsi> void f() { } //expected-note{{ignored}}

int main() {
  constexpr v4si vsi = (v4si){1, 2, 3, 4};
  f<vsi>(); //expected-error{{sorry}} expected-error{{no matching}}
  return 0; 
}
} // end ns1
} // test_unimplemented
namespace bug_clang_trunk_strings_as_default_member_initializers {

constexpr struct C {
  char X[4] = "abc";
} GC{};

} //end ns bug_clang_trunk_strings_as_default_member_initializers

