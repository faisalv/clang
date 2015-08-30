//===--- RecursiveASTVisitor.h - Recursive AST Visitor ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the APValueVisitor interface, which recursively
//  traverses APValues.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_AST_APVALUEVISITOR_H
#define LLVM_CLANG_AST_APVALUEVISITOR_H
#include "AST.h"
#include "APValue.h"

namespace clang {

template<class DerivingClass, bool IsConst = true>
class APValueVisitor {

  // Selects the current class if 'void' is supplied as the template argument to
  // DerivingClass.
  template<class CurTy, class DerivedTy> struct ImplPicker {
    using type = DerivedTy;
  };
  template<class CurTy> struct ImplPicker<CurTy, void> {
    using type = CurTy;
  };
protected:
  using APValueParamTy = // const APValue &
      typename std::conditional<IsConst, const APValue &, APValue &>::type;
  using ImplClass = // APValueVisitor;
      typename ImplPicker<APValueVisitor, DerivingClass>::type;
  // A convenient typedef to be used by the Derived classes to refer to base
  // functionality
  using BaseTy = APValueVisitor;
  ImplClass &getDerived() { return *static_cast<ImplClass *>(this); }

// A helper macro to implement short-circuiting when recursing.  It
// invokes CALL_EXPR, which must be a method call, on the derived
// object (s.t. a user of RecursiveASTVisitor can override the method
// in CALL_EXPR).

#define TRY_TO(CALL_EXPR)                                                      \
  do {                                                                         \
    if (!getDerived().CALL_EXPR)                                               \
      return false;                                                            \
  } while (0)

#define UNIMPLEMENTED_VISITOR(X) \
  llvm_unreachable("APValue Visitor for '" #X "' is unimplemented")

public:
  bool isStringLiteral(const APValue &V) {
    const Expr *E;
    if (V.isLValue() && (E = V.getLValueBase().dyn_cast<const Expr*>()))
      return isa<StringLiteral>(E->IgnoreParens());
    return false;
  }

  bool VisitLValueDeclArraySubObj(APValueParamTy AP, const ValueDecl *RootDecl,
                                   const ValueDecl *CurArrayDecl,
                                   QualType ElementTy, unsigned ArrayIndex,
                                   unsigned PathIndex, unsigned NumPaths) {

    return true;
  }

  bool VisitLValueDeclBaseSubObj(APValueParamTy AP, const ValueDecl *RootDecl,
                                  const CXXRecordDecl *ParentClass,
                                  const CXXRecordDecl *BaseClass,
                                  unsigned PathIndex, unsigned NumPaths) {

    return true;
  }

  bool VisitLValueDeclFieldSubObj(APValueParamTy AP, const ValueDecl *RootDecl,
                                   const FieldDecl *FD, unsigned PathIndex,
                                   unsigned NumPaths) {

    return true;
  }
  
  bool VisitLValueRootDecl(APValueParamTy AP, const ValueDecl *RootDecl) {
    return true;
  }

  bool VisitLValueExprArraySubObj(APValueParamTy AP, const Expr *RootExpr,
                                   const ValueDecl *CurArrayDecl,
                                   QualType ElementTy, unsigned ArrayIndex,
                                   unsigned PathIndex, unsigned NumPaths) {

    return true;
  }

  bool VisitLValueExprBaseSubObj(APValueParamTy AP, const Expr *RootExpr,
                                  const CXXRecordDecl *ParentClass,
                                  const CXXRecordDecl *BaseClass,
                                  unsigned PathIndex, unsigned NumPaths) {

    return true;
  }

  bool VisitLValueExprFieldSubObj(APValueParamTy AP, const Expr *RootExpr,
                                   const FieldDecl *FD, unsigned PathIndex,
                                   unsigned NumPaths) {

    return true;
  }
  bool VisitLValueRootExpr(APValueParamTy AP, const Expr *RootExpr) {
    return true;
  }

  bool VisitLValueDecl(APValueParamTy AP, const ValueDecl *VD) {
    
    TRY_TO(VisitLValueRootDecl(AP, VD));

    if (!AP.hasLValuePath()) return true;

    ArrayRef<APValue::LValuePathEntry> SubObjPaths = AP.getLValuePath();
    QualType SubTy = VD->getType().getCanonicalType();
    unsigned PathIdx = 0;
    const int NumSubObjPaths = SubObjPaths.size();
    const ValueDecl *CurArrayDecl = SubTy->isArrayType() ? VD : nullptr;

    
    for (auto &&LE : SubObjPaths) {
      if (auto *RD = SubTy->getAsCXXRecordDecl()) {
        const Decl *BaseOrMember =
            APValue::BaseOrMemberType::getFromOpaqueValue(LE.BaseOrMember)
                .getPointer();
        if (const CXXRecordDecl *Base = dyn_cast<CXXRecordDecl>(BaseOrMember)) {
          TRY_TO(VisitLValueDeclBaseSubObj(
              AP, VD, RD, Base, PathIdx, NumSubObjPaths));
        } else {
          // Does this handle indirect field decl
          assert(isa<FieldDecl>(BaseOrMember));
          const FieldDecl *MemD = cast<FieldDecl>(BaseOrMember);
          SubTy = MemD->getType().getCanonicalType();
          TRY_TO(VisitLValueDeclFieldSubObj(
              AP, VD, MemD, PathIdx, NumSubObjPaths));
          if (SubTy->isArrayType())
            CurArrayDecl = MemD;
        }
      } else {
        assert(SubTy->isArrayType());
        auto *CAT = cast<ConstantArrayType>(SubTy.getTypePtr());
        SubTy = CAT->getElementType();
        TRY_TO(VisitLValueDeclArraySubObj(
            AP, VD, CurArrayDecl, SubTy, LE.ArrayIndex, PathIdx, NumSubObjPaths));
      }
      ++PathIdx;
    }
    return true;
  }

  bool VisitLValueExpr(APValueParamTy AP, const Expr *E) {
    
    TRY_TO(VisitLValueRootExpr(AP, E));
    
    if (!AP.hasLValuePath()) return true;
    ArrayRef<APValue::LValuePathEntry> SubObjPaths = AP.getLValuePath();
    QualType SubTy = E->getType().getCanonicalType();
    unsigned PathIdx = 0;
    const int NumSubObjPaths = SubObjPaths.size();
    const ValueDecl *CurArrayDecl = nullptr;
    // Go through each subobject designation path.
    for (auto &&LE : SubObjPaths) {
      if (auto *RD = SubTy->getAsCXXRecordDecl()) {
        const Decl *BaseOrMember =
            APValue::BaseOrMemberType::getFromOpaqueValue(LE.BaseOrMember)
                .getPointer();
        if (const CXXRecordDecl *Base = dyn_cast<CXXRecordDecl>(BaseOrMember)) {
          TRY_TO(VisitLValueExprBaseSubObj(
              AP, E, RD, Base, PathIdx, NumSubObjPaths));
        } else {
          // Does this handle indirect field decl
          assert(isa<FieldDecl>(BaseOrMember));
          const FieldDecl *MemD = cast<FieldDecl>(BaseOrMember);
          SubTy = MemD->getType().getCanonicalType();
          TRY_TO(VisitLValueExprFieldSubObj(
              AP, E, MemD, PathIdx, NumSubObjPaths));
          if (SubTy->isArrayType())
            CurArrayDecl = MemD;
        }
      } else {
        assert(SubTy->isArrayType());
        auto *CAT = cast<ConstantArrayType>(SubTy.getTypePtr());
        SubTy = CAT->getElementType();
        TRY_TO(VisitLValueExprArraySubObj(
            AP, E, CurArrayDecl, SubTy, LE.ArrayIndex, PathIdx, NumSubObjPaths));
      }
      ++PathIdx;
    }
    return true;
  }
  bool VisitNullPtr(APValueParamTy AP) {
    return true;
  }
  bool VisitLValue(APValueParamTy AP) {
   
    const APValue::LValueBase LVB = AP.getLValueBase();
    
    if (LVB.is<const Expr *>()) {
      auto *E = LVB.dyn_cast<const Expr *>();
      if (!E) {
        TRY_TO(VisitNullPtr(AP));
      } else {
        TRY_TO(VisitLValueExpr(AP, E));
      }
    } else {
      const ValueDecl *VD = LVB.get<const ValueDecl *>();
      assert(VD);
      TRY_TO(VisitLValueDecl(AP, VD));
    }
    return true;
  }
  #define DISPATCH(APVALTYPE)                             \
  case APValue::APVALTYPE:                                \
    TRY_TO(Visit ## APVALTYPE(static_cast<APValueParamTy>(AP))); \
    break;                                                \
    /**/

  bool Visit(APValueParamTy AP) {
    switch(AP.getKind()) {
      DISPATCH(Uninitialized);
      DISPATCH(Int);
      DISPATCH(Float);
      DISPATCH(ComplexInt);
      DISPATCH(ComplexFloat);
      DISPATCH(LValue);
      DISPATCH(Vector);
      DISPATCH(Array);
      DISPATCH(Struct);
      DISPATCH(Union);
      DISPATCH(MemberPointer);
      DISPATCH(AddrLabelDiff);
    }
    return true;
  }
  bool VisitUninitialized(APValueParamTy AP) {
    return true;
  }
  bool VisitInt(APValueParamTy AP) {
    return true;
  }
  bool VisitFloat(APValueParamTy AP) {
    return true;
  }
    
  bool VisitArrayElement(APValueParamTy ElementAP, const bool IsFiller,
                         const unsigned ElementIdx, const unsigned ArraySize) {
    TRY_TO(Visit(ElementAP));
    return true;
  }

  bool VisitArray(APValueParamTy AP) {
    const unsigned ArraySize = AP.getArraySize();

    const unsigned NumInitializedElts = AP.getArrayInitializedElts();
    unsigned I;
    for (I = 0; I != NumInitializedElts; ++I) {
      TRY_TO(VisitArrayElement(AP.getArrayInitializedElt(I),
                               /*IsFiller*/ false, I, ArraySize));
    }
    // Add the fillers ... so that explicitly and implicitly initialized zeros
    // hash to the same.
    while (I++ != ArraySize) {
      TRY_TO(VisitArrayElement(AP.getArrayFiller(), /*IsFiller*/ true, I,
                               ArraySize));
    }
    return true;
  }
  bool VisitStructBase(APValueParamTy BaseAP, const CXXRecordDecl *ParentClass,
                       const CXXBaseSpecifier *BaseSpecifier,
                       unsigned BaseIndex) {
    TRY_TO(Visit(BaseAP));
    return true;
  }
  bool VisitStructField(APValueParamTy FieldAP, const FieldDecl *FD) {
    TRY_TO(Visit(FieldAP));
    return true;
  }

  bool VisitStruct(APValueParamTy AP) {
    const RecordDecl *CStructRD = AP.getStructDecl();
    const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(CStructRD);
    // Only if we have a CXXRecordDecl do we need to visit bases ...
    if (RD) {
      const unsigned NumBases = AP.getStructNumBases();
      // Vist all the bases first
      auto *baseIt = RD->bases_begin();
      assert(NumBases == RD->getNumBases());
      for (unsigned I = 0; I != NumBases; ++I, ++baseIt) {
        TRY_TO(VisitStructBase(AP.getStructBase(I), RD, baseIt, I));
      }
    }
    const unsigned NumFields = AP.getStructNumFields();
    auto fieldIt = CStructRD->field_begin();
    // Visit all the data members
    for (unsigned I = 0; I != NumFields; ++I, ++fieldIt) {
      TRY_TO(VisitStructField(AP.getStructField(I), *fieldIt));
    }
    assert(fieldIt == CStructRD->field_end());
    return true;
  }

  bool VisitUnionField(APValueParamTy FieldAP, const FieldDecl *FD) {
    TRY_TO(Visit(FieldAP));
    return true;
  }

  bool VisitUnion(APValueParamTy AP) {
    TRY_TO(VisitUnionField(AP.getUnionValue(), AP.getUnionField()));
    return true;
  }
  bool VisitMemberPointer(APValueParamTy AP) {
    return true;
  }
  bool VisitVector(APValueParamTy AP) {
    UNIMPLEMENTED_VISITOR(Vector);
    return true;
  }
  bool VisitComplexInt(APValueParamTy AP) {
    UNIMPLEMENTED_VISITOR(ComplexInt);
    return true;
  }
  bool VisitComplexFloat(APValueParamTy AP) {
    UNIMPLEMENTED_VISITOR(ComplexFloat);
    return true;
  }

  bool VisitAddrLabelDiff(APValueParamTy AP) {
    UNIMPLEMENTED_VISITOR(ComplexFloat);
    return true;
  }

#undef TRY_TO
#undef DISPATCH
#undef UNIMPLEMENTED_VISITOR
};



} // end namespace clang

#endif // LLVM_CLANG_AST_APVALUEVISITOR_H
