// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only -fufc %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fufc %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions -fufc %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions -fufc %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING



// Separate this test out for delayed template parsing since it disables other tests because it introduces a fatal error that silences others

namespace must_be_last_test_when_recursive_call {
  template<typename T>
  auto begin(T &&t) -> decltype(t.begin()) { return t.begin(); } //expected-error{{recursive template instantiation}} \
                                                                 //expected-note 9{{while substituting}} \
                                                                 //expected-note {{skipping}}
  struct A { };
  void test() {
    begin(A{}); //expected-note{{while substituting}}
  }
} // end test_if_recursive_call
} //end ns recursion
