// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only -fufc-favor-as-written %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fufc-favor-as-written %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions -fufc-favor-as-written %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions -fufc-favor-as-written %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only -fufc-favor-member %s -DUFC_FAVOR_MEMBER
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fufc-favor-member %s -DDELAYED_TEMPLATE_PARSING -DUFC_FAVOR_MEMBER
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions -fufc-favor-member %s -DMS_EXTENSIONS -DUFC_FAVOR_MEMBER
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions -fufc-favor-member %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING -DUFC_FAVOR_MEMBER



namespace ns1 {

struct X { };
constexpr int foo(X) { return 2; }
static_assert(X{}.foo() == 2, "");

} //end ns1


namespace ns2 {
struct X;
template<class> int *f(X);

struct X {
  template<int> char *f();
};

char *c = f<3>(X{});
int *v = f<int>(X{});

} // end ns2

namespace ns3 {

struct X;
template<class> int *f(X);

struct X {
  template<int> char *f();
};

template<class T> void make_calls(T t) {
  char *c = f<3>(t);
  int *v = f<T>(t);
}

template void make_calls(X);


} // end ns3

namespace implicit_member_call {
namespace ns1 {
struct Y {
  int *f(); 
};
struct X : Y { 
  private:
    void f(Y);
  
  public:
    void mf() const {
      int *p = f(Y{});
    }
};
} // end ns

} // end ns implicit_member_call


namespace disable_adl {
namespace ns1 {
  namespace inner {
    struct X { short *h(int); double *j(const void*); };
    char *f(X); 
    
    bool *g(X&&);
    float *h(X&&, int);
    long  *j(X, const char*); //expected-note {{ declared here}}
  } //end ns inner

  int *f(inner::X); 
  char *g(const inner::X&);
  
  void run() {
    // adl finds both, so error
    inner::X{}.f(); //expected-error{{no member}} 
    int *no_adl_yes_ufc = (inner::X{}.f)();
    char *no_ufc = ((inner::X{}.f))(); //expected-error{{no member}}
    // adl finds the better match - rvalue ref
    bool *yes_adl = g(inner::X{});
    // disable adl, only consider the const lvalue ref
    char *no_adl = (g)(inner::X{});
    // re-enable-adl with two parens, disable ufc
    bool *yes_adl2 = ((g))(inner::X{});
#ifdef UFC_FAVOR_MEMBER    
    short *fvmem1 = h(inner::X{}, 3);
    { double *x = j(inner::X{}, ""); }    
#else
    float *yes_adl3 = h(inner::X{}, 3);
    { long *x = j(inner::X{}, ""); }    
#endif
    short *no_adl_yes_ufc2 = (h)(inner::X{}, 3);
    float *yes_adl_no_ufc74 = ((h))(inner::X{}, 3);
    
    { double *x = (j)(inner::X{}, ""); }    
    { long *x = ((j))(inner::X{}, ""); }  // re-enable adl
    { long *x = (((j)))(inner::X{}, ""); } //expected-error{{use of undeclared identifier}}     
    
  }
} //end ns1


} // end ns disable_adl


namespace check_adl_ord_for_mem_fun {
namespace ns1 {
namespace cad {
  struct X { };
  char *f(X, int);
  char *g(X, double);
}

int *f(cad::X, double);
int *g(cad::X, int);
double get(cad::X);

void foo() {
  // These should find the functions that were visible during definition context lookup  
  int *p = cad::X{}.f(3.14);
  // turn off adl, use ordinary lookup only
  int *p3 = (cad::X{}.g)(3.14); //expected-warning{{implicit}}
}

template<class T> void mt(T t) {
  // These should find the functions that were visible during definition context lookup  
  int *p = t.f(3.14);
  int *p2 = cad::X{}.f(t.get());
  // turn off adl, use ordinary lookup only
  //char *p3 = (t.g)(3.14);
}

int main() {
  mt(cad::X{});
  return 0;
}
} //end ns1

namespace ns2 {
namespace cad {
  struct X { };
  char *f(X, int);
  char *g(X, double);
}

int *f(cad::X, double);
int *g(cad::X, int);
double get(cad::X);

void foo() {
  // These should find the functions that were visible during definition context lookup  
  int *p = cad::X{}.f(3.14);
  // turn off adl, use ordinary lookup only
  int *p3 = (cad::X{}.g)(3.14); //expected-warning{{implicit}}
  char *p4 = cad::X{}.g(3.14);
}

template<class T> void mt(T t) {
  // These should find the functions that were visible during definition context lookup  
  T *f(T&, double);
  T **f(T&&, double);
  // ADL is disabled for f, but still enabled for 'g'
  T *p = t.f(3.14);
  T **p2 = cad::X{}.f(t.get());
  // bring g(int)
  using ns2::g;
  char *g1 = cad::X{}.g(3.14);
  { char *g2 = t.g(3.14); }
  
  int **g(T, double);
  int **g2 = t.g(3.14);
  
  ((cad::X{}.f))(t.get()); //expected-error{{no member}}
  
}

int main() {
  mt(cad::X{});
  return 0;
}


} //end ns2

} // end ns check_adl_ord_for_mem_fun

namespace test_qualified_names_turn_off_adl_and_ufc {

namespace ns1 {

  namespace inner {
    struct X { void g(int); }; //expected-note 2{{declared here}}
       
    bool *g(X&&);
  } //end ns inner

  
char *g(const inner::X&);
  
void run() {
    // re-enable-adl with two parens, disable ufc
    bool *yes_adl2 = ((g))(inner::X{});
    // disable adl when transposing.
    char *b = (inner::X{}.g)();
    char *n = inner::X{}.inner::X::g(); //expected-error{{too few arguments}}
    char *n3 = ((inner::X{}.g))(); //expected-error{{too few arguments}}
    
    bool *n2 = inner::X{}.g();
}
  
} // ens ns1
} // end test_qualified_names_turn_off_adl_and_ufc


namespace test_template_id_matches {
namespace ns1 {
struct X;
template<class T> bool* f(X&&, T t);
struct X {
  template<int N> char* f(int);
};
// explicit template-id must match
bool *b = X{}.f<int>(5);
char *c = f<5>(X{}, 5);


} // end ns1

} // end test_template_id_matches
namespace test_operator_id_calls {

namespace ns1 {
struct X { };
char* operator+(X, X);

char *pc = X{}.operator+(X{});
} //end ns1

namespace ns2 {
struct X { 
char* operator+(X);
};
char *pc = operator+(X{}, X{});
} //end ns2

namespace ns3 {
struct X { };
X& operator*(X);

X &xr = X{}.operator*();

} //end ns3

namespace ns4 {
struct X { X& operator*(); };
X &xr = operator*(X{});

} //end ns4



} // end test_operator_and_literal_id_calls
namespace test_pointers {

namespace ns1 {
struct X {
};

char *f(X&);

void run() {
  X x;
  // Dereference the object expression and call a non-member function
  (&x)->f();
}

} // end ns1

namespace ns2 {

struct X {
  X* operator->();
};

char *f(X&);

void run() {
  X x;
  x->f();
}

} // end ns 2

namespace ns3 {

struct X { char *f(); };

void run() {
  X x{};
  f(&x);
}

}  //end ns3

namespace ns4 {

struct X { char *f(); };
template<class T> double* g(T&, int, double);

void run() {
  X x{};
  X *xp = &x;
  f(&x);
  double *dp = xp->g(5, 3.14);
}

}  //end ns4

} // end test_pointers
namespace test_lambdas_fptrs {
namespace ns1 {
  struct X { };
  void run() {
    auto f = [](X x, auto a) { return (decltype(a)*)nullptr; };
    double *d = X{}.f(3.14);
    const char **pc = X{}.f("abc");
  }
  template<class T> void runT() {
    auto f = [](T x, auto a) { return (decltype(a)*)nullptr; };
    double *d = T{}.f(3.14);
    const char **pc = T{}.f("abc");
  }
  template void runT<X>();
} // end ns1

namespace ns2 {
  struct X { void (*f)(int); };
  void run() {
    auto f = [](X x, auto a) { return (decltype(a)*)nullptr; };
    double *d = X{}.f(3.14); //expected-error{{type 'void'}}
    X{}.f("abc");
  }
  template<class T> void runT() {
    auto f = [](T x, auto a) { return (decltype(a)*)nullptr; };
    double *d = T{}.f(3.14); // expected-error{{type 'void'}}
  }
  template void runT<X>(); // expected-note{{instantiation}}
  

} // end ns2

namespace ns3 {
struct Y { void f(); };

struct X { void (*f)(int); 
  void mf() {
    f(Y{}); // should call the member function in Y
  }
}; 

} // end ns3

namespace ns4 {
namespace N {
  struct X { 
    bool *f(int); 
    int *L(int);
  };
  long *f(N::X, int);
  long *L(N::X, int);
} 
char* (*f)(N::X, int*);
auto L = [](auto a, int*) { return (decltype(a)*)nullptr; };
void test() {
  // ADL is disabled by the function ptr, but find the member function
  bool *B = f(N::X{}, 5);
  int *Mem = L(N::X{}, 6);
}
} // end ns4
} // end ns test_lambdas_fptrs


namespace test_non_class_object_member_syntax {
namespace ns1 {
constexpr int add(int x, int y) { return x + y; }

void test() {
  constexpr int X = 10;
  static_assert(X.add(5) == 15, "");
}
} // end ns1

namespace ns2 {

constexpr char index(const char *pc, int I) { return pc[I]; }
static_assert("abc".index(0) == 'a', "");

}
namespace ns3 {
constexpr int deref_add(const int &c, int n) { return c + n; }
constexpr int X = 10;
constexpr int const *pX = &X;
void test() {
  static_assert(pX->deref_add(3) == 13, "");
}
} //end ns3

namespace test_literals_ns4 {
bool *f(bool);
char *f(char);
decltype(nullptr) *f(decltype(nullptr));

bool *B = true.f();
char *C = 'a'.f();
//decltype(nullptr) *N = nullptr.f();

} // end test_literals_ns4

namespace ns5 {
struct X { X* foo(); };
double* foo(double);
int main() {
  //float f = 5.foo();
  //0.15. fo'oe+. ham e+5();
  X x;
  X* xp = x . foo();
  double *d = (5.23e+10).foo();
  (((5.23e+10).foo))(); //expected-error{{not a structure}}
}


} // end ns5
} // ns test_non_class_object_member_syntax

namespace dont_invoke_ctors {

struct Y { };
struct A {
  A(Y);
};
void test() {
  Y{}.A(); //expected-error{{no member}}
}

} //end ns dont_invoke_ctors

namespace test_obj_call {
namespace ns1 {
struct X {
  int *fn(int, int);
};
 
template<class F, class O> 
void call(F fn, O obj) {
    int *ip = fn(obj, 1, 2);  // if fn(obj) fails try obj.fn(1, 2)
};

int main() {
  call(X{}, X{});
  return 0;
}
}// end ns1

namespace ns2 {

struct X {
  int *fn(int, int);
  
  template<class ... Ts>
  char *operator()(Ts ...);
};
 
template<class F, class O> 
void call(F fn, O obj) {
#ifdef UFC_FAVOR_MEMBER
  int *p = fn(obj, 1, 1);
#else
  char *pc = fn(obj, 1, 2);  // if fn(obj) fails try obj.fn(1, 2)
#endif
};

int main() {
  call(X{}, X{});
  return 0;
}
}// end ns2



} // end ns test_obj_call
namespace test_recursion {

namespace test_disable_udl_recursive_call {
  template<typename T>
  auto begin(T &&t) -> decltype(((t.begin))()) { return t.begin(); } //expected-note{{ignored}}
  
  struct A { };
  
  void test() {
    begin(A{});  //expected-error{{no matching function}} 
  }
} // end test_disable_udl_recursive_call

} //end ns recursion
