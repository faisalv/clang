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

#define H2(X, Y, ...) __VA_OPT__(X ## Y,) __VA_ARGS__

15: H2(a, b, c, d) // replaced ab, c, d
// CHECK: 15: ab, c, d

#undef X
#undef V

#define V(a,...) __VA_OPT__(__VA_ARGS__)

16: V(a,)
17: V(a,,)
// CHECK: 16: 
// CHECK: 17: ,

#undef X
#undef V

#define X 123
#define Y xyz
#define V(...) AB ## __VA_OPT__(__VA_ARGS__ __VA_ARGS__ __VA_ARGS__) ## BA

18: V(X)
// CHECK: 18: ABX 123 XBA  
19: V(X, Y)
// CHECK: 19: ABX, xyz 123, xyz 123, YBA  
#undef V
#undef X
#undef Y


#undef FOO
#define FOO(x,...) # __VA_OPT__(x) #x #__VA_OPT__(__VA_ARGS__) 
20: FOO(1)
// CHECK: 20: "" "1" ""
21: FOO(1,2,3)
// CHECK: 21: "1" "1" "2,3"

#undef FOO
#define FOO(x,...) x __VA_OPT__(##) __VA_ARGS__
22: FOO(1)
// CHECK: 22: 1
23: FOO(1,2,3)
// CHECK: 23: 12,3

#undef FOO
#define FOO(x,...) __VA_OPT__(#) __VA_OPT__(x) __VA_OPT__(#) __VA_OPT__(__VA_ARGS__) # __VA_OPT__(x)
24: FOO(1) END
// CHECK: 24: "" END
25: FOO(1,2,3) END
// CHECK: 25: "1" "2,3" "1" END

#undef FOO
#define FOO(x,...) __VA_OPT__(#) x __VA_OPT__(#) __VA_ARGS__

26: FOO(1) END
// CHECK: 26: 1 END
27: FOO(1,2,  3) END
// CHECK: 27: "1" "2, 3" END


#undef FOO
#define FOO(x,...) __VA_OPT__(#) __VA_OPT__(x)

28: FOO(1) END
// CHECK: 28: END
29: FOO(1,2,  3) END
// CHECK: 29: "1" END

#undef FOO
#define FOO(x,...) __VA_OPT__() __VA_OPT__(##) x 
#define X() expandedX
30: FOO(X LPAREN RPAREN) END
// CHECK: 30: expandedX END
31: FOO(X LPAREN RPAREN, 1) END
// CHECK: 31: X ( ) END

#undef X
#undef FOO
#define FOO(x,y,...) x __VA_OPT__(##) __VA_OPT__(__VA_ARGS__ ##) y __VA_OPT__(##) __VA_ARGS__

32: FOO(1,2,) END
// CHECK: 32: 1 2 END
33: FOO(1,2, 3, 4) END
// CHECK: 33: 13, 423, 4 END
