//===--- LangOptions.cpp - C Language Family Language Options ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the LangOptions class.
//
//===----------------------------------------------------------------------===//
#include "clang/Basic/LangOptions.h"

using namespace clang;

LangOptions::LangOptions() {
#define LANGOPT(Name, Bits, Default, Description) Name = Default;
#define ENUM_LANGOPT(Name, Type, Bits, Default, Description) set##Name(Default);
#include "clang/Basic/LangOptions.def"
}

bool LangOptions::isUnifiedFunctionCallEnabled() const {
  return CPlusPlus && (UFCFavorAsWritten ||
         UFCFavorMember);
}

bool LangOptions::isUFCWarnAboutTranspose() const {
  return UFCWarnAboutTranspose;
}

bool LangOptions::isUFCFavorAsWritten() const {
    return UFCFavorAsWritten;
  }
bool LangOptions::isUFCFavorMember() const {
    return //isUnifiedFunctionCallEnabled();
     UFCFavorMember;
}
bool LangOptions::isUFCTreatObjectWithArrowAsPointer() const {
  return 
    UFCTreatObjectWithArrowOpAsPointer;
}

bool LangOptions::isUFCStatsOn() const {
  return UFCStats;
}

void LangOptions::resetNonModularOptions() {
#define LANGOPT(Name, Bits, Default, Description)
#define BENIGN_LANGOPT(Name, Bits, Default, Description) Name = Default;
#define BENIGN_ENUM_LANGOPT(Name, Type, Bits, Default, Description) \
  Name = Default;
#include "clang/Basic/LangOptions.def"

  // FIXME: This should not be reset; modules can be different with different
  // sanitizer options (this affects __has_feature(address_sanitizer) etc).
  Sanitize.clear();
  SanitizerBlacklistFiles.clear();

  CurrentModule.clear();
  ImplementationOfModule.clear();
}

