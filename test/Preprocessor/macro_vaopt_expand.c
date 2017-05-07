// RUN: %clang_cc1 -E %s | FileCheck -strict-whitespace %s

#define LPAREN ( 
#define RPAREN ) 
#define F(x, y) x + y 
#define ELLIP_FUNC(...) __VA_OPT__(__VA_ARGS__)

1: ELLIP_FUNC(F, LPAREN, 'a', 'b', RPAREN); /* 1st invocation */ 
2: ELLIP_FUNC(F LPAREN 'a', 'b' RPAREN); /* 2nd invocation */ 

// CHECK: 1: F, (, 'a', 'b', );
// CHECK: 2: 'a' + 'b';
#undef F

#define F(...) f(0 __VA_OPT__(,) __VA_ARGS__)
3: F(a, b, c) // replaced by f(0, a, b, c) 
4: F() // replaced by f(0)

// CHECK: 3: f(0 , a, b, c) 
// CHECK: 4: f(0 )

#define G(X, ...) f(0, X __VA_OPT__(,) __VA_ARGS__)

5: G(a, b, c) // replaced by f(0, a , b, c) 
6: G(a) // replaced by f(0, a) 
7: G(a,) // replaced by f(0, a) 

// CHECK: 5: f(0, a , b, c) 
// CHECK: 6: f(0, a ) 
// CHECK: 7: f(0, a ) 


#define SDEF(sname, ...) S sname __VA_OPT__(= { __VA_ARGS__ })

8: SDEF(foo); // replaced by S foo; 
9: SDEF(bar, 1, 2); // replaced by S bar = { 1, 2 }; 

// CHECK: 8: S foo ;
// CHECK: 9: S bar = { 1, 2 }; 


10: G(a,,)
// CHECK: 10: f(0, a , ,)

#undef X
#undef V

#define X 123
#define V(x, ...) AB ## __VA_OPT__(x ## x x) x

11: V(X, xyz)
// CHECK: 11: ABXX 123 123
12: V(X)
// CHECK: 12: AB 123
#undef X
#undef V

#define X 123
#define V(x, ...) AB ## __VA_OPT__(x x x) ## __VA_ARGS__

13: V(X, BA)
14: V(X)

// CHECK: 13: ABX 123 XBA  
// CHECK: 14: AB  

