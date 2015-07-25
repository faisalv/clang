// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only -fufc  %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fufc %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions -fufc %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions -fufc  %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only -fufc -fufc-arrowobj %s -DARROWOBJ_AS_PTR
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fufc -fufc-arrowobj %s -DDELAYED_TEMPLATE_PARSING -DARROWOBJ_AS_PTR
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions -fufc -fufc-arrowobj %s -DMS_EXTENSIONS -DARROWOBJ_AS_PTR
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions -fufc -fufc-arrowobj %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING -DARROWOBJ_AS_PTR


namespace ns1 {
struct Y {
  char *foo();
};
struct Z {
  float *foo();
};
struct X { 
  Y* operator->();
  Z* operator->() const;
  int *foo();
};

void main() {
#ifdef ARROWOBJ_AS_PTR
  char *c = foo(X{});
  const X xc{};
  float *f = foo(xc);
#else  
  char *c = foo(X{}); //expected-error{{rvalue of type 'int *'}}
#endif
}


} // end ns1

namespace ns2 {
struct Y {
  float *foo();
};
struct X { 
  Y* operator->();
  int *foo();
};

void main() {

#ifdef ARROWOBJ_AS_PTR
  const X xc{};
  float *f = foo(xc); //expected-error{{use of undeclared identifier 'foo'}}
  float *f2 = foo(X{});
#else  
  int *i = foo(X{});
#endif
}


} // end ns1

