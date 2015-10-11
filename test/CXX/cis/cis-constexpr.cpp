// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING


namespace test_constexpr_ns {

namespace test_nested_unions {
  union A { int ai; char ac = 'a'; };
  union B { int bi; char bc; };
  union U {
    A ua;
    B ub;
  };
  
  constexpr U u{};
  static_assert(u.ub.bc == 'a', "");
  constexpr A a = u.ua;
  constexpr B b = u.ub; //expected-error{{must be initialized by a constant expression}} expected-note{{active member}} expected-note{{in call}}
  namespace test_union_order_matters {
    union A { int ai; char ac = 'a'; };
    union B { char bc; int bi;  };
    union U {
      A ua;
      B ub;
    };
    constexpr U u{};
    constexpr char C = u.ub.bc;  //expected-error{{must be initialized by a constant expression}} expected-note{{active member}} 
    constexpr A a = u.ua; 
  
  } // end ns0_1
} // end ns0

namespace ns1 {
  struct A {
    int ai;
    char ac;
    double *dptr;
  };
  struct B {
    int bi;
    char bc;
    double d;
  };
  union U {
    A ua;
    B ub;
  };
  
  constexpr U u{{1,'a',nullptr}};
  static_assert(u.ub.bc == 'a', "");
} //end ns1

namespace ns2 {
  union A { constexpr A() : y(5) {} int x, y; };
  struct B { A a; };
  struct C : B {};
  union D { constexpr D() : c() {} constexpr D(int n) : n(n) {} C c; int n; };
  
  constexpr bool i() { 
    D d(0); 
    int I = d.c.a.x; //expected-note{{read of member 'c'}}
    return true;
  } 
  static_assert(i(), ""); // expected-error {{constant expression}} expected-note {{in call}}
} // end ns2

namespace ns3 {

struct A { int ai; double ad; };
struct B { int bi; double bd; char c; };
struct AA { A aa; };
struct BB { B bb; };
constexpr union U {
  AA ua;
  BB ub;
} u{ {1, 3.14} };

constexpr B b = u.ub.bb; //expected-error{{must be initialized by a constant expression}} expected-note{{active member}} expected-note{{in call}}
} // end ns3

namespace ns4 {
struct A { int ai; double ad; };
struct B { int bi; double bd; };
struct AA { A aa; };
struct BB { B bb; };
constexpr union U {
  AA ua;
  BB ub;
} u{ {1, 3.14} };

constexpr B b = u.ub.bb; 

} //end ns4

namespace ns5 {
struct A { int a; char c; };
struct B { int b; double d; };
union U { A a; B b; } constexpr u = U();
  
constexpr const int *bp = &u.b.b;
constexpr int b = u.b.b;  

} // end ns5

namespace ns6 {
union V1 {
  char c = 'a';
  int i;
};
union V2 {
  char c2 = 'b';
  int j;
  double d;
};

struct A { int I; char c; V1 v1;};
struct B { int X; char J; V2 v2;};

union U {
  A a;
  B b;
};

constexpr union U u{{3}};
static_assert(u.b.v2.c2 == 'a', "");

}  // end ns6

namespace ns7_unions_must_be_in_same_order {
union V1 {
  char c = 'a';
  int i;
};
union V2 {
  int j;
  char c2 = 'b';
  double d;
};

struct A { int I; char c; V1 v1;};
struct B { int X; char J; V2 v2;};

union U {
  A a;
  B b;
};

constexpr union U u{{3}};
static_assert(u.b.v2.c2 == 'a', ""); //expected-error{{not an integral constant expression}} //expected-note{{active member 'c'}}


} // end ns7_unions_must_be_in_same_order

namespace ns8_bit_fields {

namespace ns1 {
union V1 {
  char c;
  char : 0;
  int i = 10;
};
union V2 {
  char c2;
  int : 0;
  int j = 5;
  double d;
};

struct A { int I; char c; V1 v1;};
struct B { int X; char J; V2 v2;};

union U {
  A a;
  B b;
};

constexpr union U u{{3}};
static_assert(u.b.v2.j == 10, ""); //expected-error{{not an integral constant expression}} //expected-note{{active member 'i'}}

} // end ns1
namespace ns2 {
union V1 {
  char c;
  int : 0;
  int i = 10;
};
union V2 {
  char c2;
  int : 0;
  int j = 5;
  double d;
};

struct A { int I; char c; V1 v1;};
struct B { int X; char J; V2 v2;};

union U {
  A a;
  B b;
};

constexpr union U u{{3}};
static_assert(u.b.v2.j == 10, ""); //OK

} // end ns2

namespace ns3 {
union V1 {
  char c;
  int : 1;
  int i = 10;
};
union V2 {
  char c2;
  int : 0;
  int j = 5;
  double d;
};

struct A { int I; char c; V1 v1;};
struct B { int X; char J; V2 v2;};

union U {
  A a;
  B b;
};

constexpr union U u{{3}};
static_assert(u.b.v2.j == 10, ""); //expected-error{{not an integral constant expression}} //expected-note{{active member 'i'}}

} // end ns3

} // bit_fields
namespace test_arrays {
union V1 {
  char c;
  int : 1;
  int i = 10;
};
union V2 {
  char c2;
  int : 1;
  int j;
  double d;
};


struct A { int I[10] = { 1, 2, 3, 4 }; char c; V1 v1[5];};
struct B { int X[10]; char J; V2 v2[5];};



namespace ns1 {
struct A { int ai; char ac; };
struct B { int bi; char bc; /*double d;*/ };


constexpr union U {
  A ua[5] = { 1, 'a', 2, 'b' };
  B ub[5];
} u{};

static_assert(u.ub[1].bc == 'b', "");
constexpr B const (*b)[5] = &u.ub;
// Can not access different indices of a union with an active index (i.e ua vs ub, unless we are diving into it to access the CIS)
constexpr B bv = (*b)[0]; //expected-error{{must be initialized by a constant expression}} expected-note{{active member}} expected-note{{in call}}
static_assert((*b)[0].bc == 'a', "");
} // end ns1

namespace ns2 {
struct A { int ai; char ac; };
struct B { int bi; char bc; double d; };


constexpr union U {
  A ua[5] = { 1, 'a', 2, 'b' };
  B ub[5];
} u{};

static_assert(u.ub[0].bc == 'a', "First element is ok!");

static_assert(u.ub[1].bc == 'b', "Second element is not given layout discrepancies"); //expected-error{{not an integral constant expression}} expected-note{{active member}} 

constexpr B const (*b)[5] = &u.ub;
static_assert((*b)[0].bc == 'a', "");
} // end ns2


} // end test_arrays
} // test_constexpr_ns

namespace test_non_standard_layout_unions {

  struct A {
    int x;
  };
  struct B {
    int y;
  private:
    double z;
  };
  union U {
    A ua;
    B ub;
  };

constexpr U u{{1}};
constexpr int i = u.ub.y; //expected-error{{must be initialized by a constant expression}} expected-note{{active member}}


}