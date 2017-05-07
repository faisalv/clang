// RUN: %clang_cc1 %s -Eonly -verify -Wno-all -pedantic

//expected-error@+1{{missing '('}}
#define V1(...) __VA_OPT__  

#undef V1
//expected-error@+1{{empty}}
#define V1(...) __VA_OPT__  ()

//expected-warning@+1{{can only appear in the expansion of a C99 variadic macro}}
#define V2() __VA_OPT__(x) 

//expected-error@+1{{missing ')'}}
#define V3(...) __VA_OPT__(

#define V4(...) __VA_OPT__(__VA_ARGS__)

//expected-error@+1{{nested}}
#define V5(...) __VA_OPT__(__VA_OPT__())

#undef V1
//expected-error@+1{{not followed by}}
#define V1(...) __VA_OPT__  (#)

#undef V1
//expected-error@+1{{cannot appear at start}}
#define V1(...) __VA_OPT__  (##)


#undef V1
//expected-error@+1{{cannot appear at start}}
#define V1(...) __VA_OPT__  (## X)

#undef V1
//expected-error@+1{{cannot appear at end}}
#define V1(...) __VA_OPT__  (X ##)


#undef V1
//expected-error@+1{{missing ')'}}
#define V1(...) __VA_OPT__  ((())

