//===--- APValue.cpp - Union class for APFloat/APSInt/Complex -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the APValue class.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/APValue.h"
#include "clang/AST/APValueVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Diagnostic.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace {
  struct LVBase {
    llvm::PointerIntPair<APValue::LValueBase, 1, bool> BaseAndIsOnePastTheEnd;
    CharUnits Offset;
    unsigned PathLength;
    unsigned CallIndex;
  };
}

struct APValue::LV : LVBase {
  static const unsigned InlinePathSpace =
      (DataSize - sizeof(LVBase)) / sizeof(LValuePathEntry);

  /// Path - The sequence of base classes, fields and array indices to follow to
  /// walk from Base to the subobject. When performing GCC-style folding, there
  /// may not be such a path.
  union {
    LValuePathEntry Path[InlinePathSpace];
    LValuePathEntry *PathPtr;
  };

  LV() { PathLength = (unsigned)-1; }
  ~LV() { resizePath(0); }

  void resizePath(unsigned Length) {
    if (Length == PathLength)
      return;
    if (hasPathPtr())
      delete [] PathPtr;
    PathLength = Length;
    if (hasPathPtr())
      PathPtr = new LValuePathEntry[Length];
  }

  bool hasPath() const { return PathLength != (unsigned)-1; }
  bool hasPathPtr() const { return hasPath() && PathLength > InlinePathSpace; }

  LValuePathEntry *getPath() { return hasPathPtr() ? PathPtr : Path; }
  const LValuePathEntry *getPath() const {
    return hasPathPtr() ? PathPtr : Path;
  }
};

namespace {
  struct MemberPointerBase {
    llvm::PointerIntPair<const ValueDecl*, 1, bool> MemberAndIsDerivedMember;
    unsigned PathLength;
  };
}

struct APValue::MemberPointerData : MemberPointerBase {
  static const unsigned InlinePathSpace =
      (DataSize - sizeof(MemberPointerBase)) / sizeof(const CXXRecordDecl*);
  typedef const CXXRecordDecl *PathElem;
  union {
    PathElem Path[InlinePathSpace];
    PathElem *PathPtr;
  };

  MemberPointerData() { PathLength = 0; }
  ~MemberPointerData() { resizePath(0); }

  void resizePath(unsigned Length) {
    if (Length == PathLength)
      return;
    if (hasPathPtr())
      delete [] PathPtr;
    PathLength = Length;
    if (hasPathPtr())
      PathPtr = new PathElem[Length];
  }

  bool hasPathPtr() const { return PathLength > InlinePathSpace; }

  PathElem *getPath() { return hasPathPtr() ? PathPtr : Path; }
  const PathElem *getPath() const {
    return hasPathPtr() ? PathPtr : Path;
  }
};

// FIXME: Reduce the malloc traffic here.

APValue::Arr::Arr(unsigned NumElts, unsigned Size) :
  Elts(new APValue[NumElts + (NumElts != Size ? 1 : 0)]),
  NumElts(NumElts), ArrSize(Size) {}
APValue::Arr::~Arr() { delete [] Elts; }

APValue::StructData::StructData(unsigned NumBases, unsigned NumFields,
                                const RecordDecl *RD) :
  Elts(new APValue[NumBases+NumFields]),
  NumBases(NumBases), NumFields(NumFields), StructDecl(RD) {}
APValue::StructData::~StructData() {
  delete [] Elts;
}

APValue::UnionData::UnionData() : Field(nullptr), Value(new APValue) {}
APValue::UnionData::~UnionData () {
  delete Value;
}

APValue::APValue(const APValue &RHS) : Kind(Uninitialized) {
  switch (RHS.getKind()) {
  case Uninitialized:
    break;
  case Int:
    MakeInt();
    setInt(RHS.getInt());
    break;
  case Float:
    MakeFloat();
    setFloat(RHS.getFloat());
    break;
  case Vector:
    MakeVector();
    setVector(((const Vec *)(const char *)RHS.Data.buffer)->Elts,
              RHS.getVectorLength());
    break;
  case ComplexInt:
    MakeComplexInt();
    setComplexInt(RHS.getComplexIntReal(), RHS.getComplexIntImag());
    break;
  case ComplexFloat:
    MakeComplexFloat();
    setComplexFloat(RHS.getComplexFloatReal(), RHS.getComplexFloatImag());
    break;
  case LValue:
    MakeLValue();
    if (RHS.hasLValuePath())
      setLValue(RHS.getLValueBase(), RHS.getLValueOffset(), RHS.getLValuePath(),
                RHS.isLValueOnePastTheEnd(), RHS.getLValueCallIndex());
    else
      setLValue(RHS.getLValueBase(), RHS.getLValueOffset(), NoLValuePath(),
                RHS.getLValueCallIndex());
    break;
  case Array:
    MakeArray(RHS.getArrayInitializedElts(), RHS.getArraySize());
    for (unsigned I = 0, N = RHS.getArrayInitializedElts(); I != N; ++I)
      getArrayInitializedElt(I) = RHS.getArrayInitializedElt(I);
    if (RHS.hasArrayFiller())
      getArrayFiller() = RHS.getArrayFiller();
    break;
  case Struct:
    MakeStruct(RHS.getStructNumBases(), RHS.getStructNumFields(),
               RHS.getStructDecl());
    for (unsigned I = 0, N = RHS.getStructNumBases(); I != N; ++I)
      getStructBase(I) = RHS.getStructBase(I);
    for (unsigned I = 0, N = RHS.getStructNumFields(); I != N; ++I)
      getStructField(I) = RHS.getStructField(I);
    break;
  case Union:
    MakeUnion();
    setUnion(RHS.getUnionField(), RHS.getUnionValue());
    break;
  case MemberPointer:
    MakeMemberPointer(RHS.getMemberPointerDecl(),
                      RHS.isMemberPointerToDerivedMember(),
                      RHS.getMemberPointerPath());
    break;
  case AddrLabelDiff:
    MakeAddrLabelDiff();
    setAddrLabelDiff(RHS.getAddrLabelDiffLHS(), RHS.getAddrLabelDiffRHS());
    break;
  }
}

void APValue::DestroyDataAndMakeUninit() {
  if (Kind == Int)
    ((APSInt*)(char*)Data.buffer)->~APSInt();
  else if (Kind == Float)
    ((APFloat*)(char*)Data.buffer)->~APFloat();
  else if (Kind == Vector)
    ((Vec*)(char*)Data.buffer)->~Vec();
  else if (Kind == ComplexInt)
    ((ComplexAPSInt*)(char*)Data.buffer)->~ComplexAPSInt();
  else if (Kind == ComplexFloat)
    ((ComplexAPFloat*)(char*)Data.buffer)->~ComplexAPFloat();
  else if (Kind == LValue)
    ((LV*)(char*)Data.buffer)->~LV();
  else if (Kind == Array)
    ((Arr*)(char*)Data.buffer)->~Arr();
  else if (Kind == Struct)
    ((StructData*)(char*)Data.buffer)->~StructData();
  else if (Kind == Union)
    ((UnionData*)(char*)Data.buffer)->~UnionData();
  else if (Kind == MemberPointer)
    ((MemberPointerData*)(char*)Data.buffer)->~MemberPointerData();
  else if (Kind == AddrLabelDiff)
    ((AddrLabelDiffData*)(char*)Data.buffer)->~AddrLabelDiffData();
  Kind = Uninitialized;
}

bool APValue::needsCleanup() const {
  switch (getKind()) {
  case Uninitialized:
  case AddrLabelDiff:
    return false;
  case Struct:
  case Union:
  case Array:
  case Vector:
    return true;
  case Int:
    return getInt().needsCleanup();
  case Float:
    return getFloat().needsCleanup();
  case ComplexFloat:
    assert(getComplexFloatImag().needsCleanup() ==
               getComplexFloatReal().needsCleanup() &&
           "In _Complex float types, real and imaginary values always have the "
           "same size.");
    return getComplexFloatReal().needsCleanup();
  case ComplexInt:
    assert(getComplexIntImag().needsCleanup() ==
               getComplexIntReal().needsCleanup() &&
           "In _Complex int types, real and imaginary values must have the "
           "same size.");
    return getComplexIntReal().needsCleanup();
  case LValue:
    return reinterpret_cast<const LV *>(Data.buffer)->hasPathPtr();
  case MemberPointer:
    return reinterpret_cast<const MemberPointerData *>(Data.buffer)
        ->hasPathPtr();
  }
  llvm_unreachable("Unknown APValue kind!");
}

void APValue::swap(APValue &RHS) {
  std::swap(Kind, RHS.Kind);
  char TmpData[DataSize];
  memcpy(TmpData, Data.buffer, DataSize);
  memcpy(Data.buffer, RHS.Data.buffer, DataSize);
  memcpy(RHS.Data.buffer, TmpData, DataSize);
}

void APValue::dump() const {
  dump(llvm::errs());
  llvm::errs() << '\n';
}

static double GetApproxValue(const llvm::APFloat &F) {
  llvm::APFloat V = F;
  bool ignored;
  V.convert(llvm::APFloat::IEEEdouble, llvm::APFloat::rmNearestTiesToEven,
            &ignored);
  return V.convertToDouble();
}

void APValue::dump(raw_ostream &OS) const {
  switch (getKind()) {
  case Uninitialized:
    OS << "Uninitialized";
    return;
  case Int:
    OS << "Int: " << getInt();
    return;
  case Float:
    OS << "Float: " << GetApproxValue(getFloat());
    return;
  case Vector:
    OS << "Vector: ";
    getVectorElt(0).dump(OS);
    for (unsigned i = 1; i != getVectorLength(); ++i) {
      OS << ", ";
      getVectorElt(i).dump(OS);
    }
    return;
  case ComplexInt:
    OS << "ComplexInt: " << getComplexIntReal() << ", " << getComplexIntImag();
    return;
  case ComplexFloat:
    OS << "ComplexFloat: " << GetApproxValue(getComplexFloatReal())
       << ", " << GetApproxValue(getComplexFloatImag());
    return;
  case LValue:
    OS << "LValue: <todo>";
    return;
  case Array:
    OS << "Array: ";
    for (unsigned I = 0, N = getArrayInitializedElts(); I != N; ++I) {
      getArrayInitializedElt(I).dump(OS);
      if (I != getArraySize() - 1) OS << ", ";
    }
    if (hasArrayFiller()) {
      OS << getArraySize() - getArrayInitializedElts() << " x ";
      getArrayFiller().dump(OS);
    }
    return;
  case Struct:
    OS << "Struct ";
    if (unsigned N = getStructNumBases()) {
      OS << " bases: ";
      getStructBase(0).dump(OS);
      for (unsigned I = 1; I != N; ++I) {
        OS << ", ";
        getStructBase(I).dump(OS);
      }
    }
    if (unsigned N = getStructNumFields()) {
      OS << " fields: ";
      getStructField(0).dump(OS);
      for (unsigned I = 1; I != N; ++I) {
        OS << ", ";
        getStructField(I).dump(OS);
      }
    }
    return;
  case Union:
    OS << "Union: ";
    getUnionValue().dump(OS);
    return;
  case MemberPointer:
    OS << "MemberPointer: <todo>";
    return;
  case AddrLabelDiff:
    OS << "AddrLabelDiff: <todo>";
    return;
  }
  llvm_unreachable("Unknown APValue kind!");
}

void APValue::printPretty(raw_ostream &Out, ASTContext &Ctx, QualType Ty) const{
  switch (getKind()) {
  case APValue::Uninitialized:
    Out << "<uninitialized>";
    return;
  case APValue::Int:
    if (Ty->isBooleanType())
      Out << (getInt().getBoolValue() ? "true" : "false");
    else
      Out << getInt();
    return;
  case APValue::Float:
    Out << GetApproxValue(getFloat());
    return;
  case APValue::Vector: {
    Out << '{';
    QualType ElemTy = Ty->getAs<VectorType>()->getElementType();
    getVectorElt(0).printPretty(Out, Ctx, ElemTy);
    for (unsigned i = 1; i != getVectorLength(); ++i) {
      Out << ", ";
      getVectorElt(i).printPretty(Out, Ctx, ElemTy);
    }
    Out << '}';
    return;
  }
  case APValue::ComplexInt:
    Out << getComplexIntReal() << "+" << getComplexIntImag() << "i";
    return;
  case APValue::ComplexFloat:
    Out << GetApproxValue(getComplexFloatReal()) << "+"
        << GetApproxValue(getComplexFloatImag()) << "i";
    return;
  case APValue::LValue: {
    LValueBase Base = getLValueBase();
    if (!Base) {
      Out << "0";
      return;
    }

    bool IsReference = Ty->isReferenceType();
    QualType InnerTy
      = IsReference ? Ty.getNonReferenceType() : Ty->getPointeeType();
    if (InnerTy.isNull())
      InnerTy = Ty;

    if (!hasLValuePath()) {
      // No lvalue path: just print the offset.
      CharUnits O = getLValueOffset();
      CharUnits S = Ctx.getTypeSizeInChars(InnerTy);
      if (!O.isZero()) {
        if (IsReference)
          Out << "*(";
        if (O % S) {
          Out << "(char*)";
          S = CharUnits::One();
        }
        Out << '&';
      } else if (!IsReference)
        Out << '&';

      if (const ValueDecl *VD = Base.dyn_cast<const ValueDecl*>())
        Out << *VD;
      else {
        assert(Base.get<const Expr *>() != nullptr &&
               "Expecting non-null Expr");
        Base.get<const Expr*>()->printPretty(Out, nullptr,
                                             Ctx.getPrintingPolicy());
      }

      if (!O.isZero()) {
        Out << " + " << (O / S);
        if (IsReference)
          Out << ')';
      }
      return;
    }

    // We have an lvalue path. Print it out nicely.
    if (!IsReference)
      Out << '&';
    else if (isLValueOnePastTheEnd())
      Out << "*(&";

    QualType ElemTy;
    if (const ValueDecl *VD = Base.dyn_cast<const ValueDecl*>()) {
      Out << *VD;
      ElemTy = VD->getType();
    } else {
      const Expr *E = Base.get<const Expr*>();
      assert(E != nullptr && "Expecting non-null Expr");
      E->printPretty(Out, nullptr, Ctx.getPrintingPolicy());
      ElemTy = E->getType();
    }

    ArrayRef<LValuePathEntry> Path = getLValuePath();
    const CXXRecordDecl *CastToBase = nullptr;
    for (unsigned I = 0, N = Path.size(); I != N; ++I) {
      if (ElemTy->getAs<RecordType>()) {
        // The lvalue refers to a class type, so the next path entry is a base
        // or member.
        const Decl *BaseOrMember =
        BaseOrMemberType::getFromOpaqueValue(Path[I].BaseOrMember).getPointer();
        if (const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(BaseOrMember)) {
          CastToBase = RD;
          ElemTy = Ctx.getRecordType(RD);
        } else {
          const ValueDecl *VD = cast<ValueDecl>(BaseOrMember);
          Out << ".";
          if (CastToBase)
            Out << *CastToBase << "::";
          Out << *VD;
          ElemTy = VD->getType();
        }
      } else {
        // The lvalue must refer to an array.
        Out << '[' << Path[I].ArrayIndex << ']';
        ElemTy = Ctx.getAsArrayType(ElemTy)->getElementType();
      }
    }

    // Handle formatting of one-past-the-end lvalues.
    if (isLValueOnePastTheEnd()) {
      // FIXME: If CastToBase is non-0, we should prefix the output with
      // "(CastToBase*)".
      Out << " + 1";
      if (IsReference)
        Out << ')';
    }
    return;
  }
  case APValue::Array: {
    const ArrayType *AT = Ctx.getAsArrayType(Ty);
    QualType ElemTy = AT->getElementType();
    Out << '{';
    if (unsigned N = getArrayInitializedElts()) {
      getArrayInitializedElt(0).printPretty(Out, Ctx, ElemTy);
      for (unsigned I = 1; I != N; ++I) {
        Out << ", ";
        if (I == 10) {
          // Avoid printing out the entire contents of large arrays.
          Out << "...";
          break;
        }
        getArrayInitializedElt(I).printPretty(Out, Ctx, ElemTy);
      }
    }
    Out << '}';
    return;
  }
  case APValue::Struct: {
    Out << '{';
    const RecordDecl *RD = Ty->getAs<RecordType>()->getDecl();
    bool First = true;
    if (unsigned N = getStructNumBases()) {
      const CXXRecordDecl *CD = cast<CXXRecordDecl>(RD);
      CXXRecordDecl::base_class_const_iterator BI = CD->bases_begin();
      for (unsigned I = 0; I != N; ++I, ++BI) {
        assert(BI != CD->bases_end());
        if (!First)
          Out << ", ";
        getStructBase(I).printPretty(Out, Ctx, BI->getType());
        First = false;
      }
    }
    for (const auto *FI : RD->fields()) {
      if (!First)
        Out << ", ";
      if (FI->isUnnamedBitfield()) continue;
      getStructField(FI->getFieldIndex()).
        printPretty(Out, Ctx, FI->getType());
      First = false;
    }
    Out << '}';
    return;
  }
  case APValue::Union:
    Out << '{';
    if (const FieldDecl *FD = getUnionField()) {
      Out << "." << *FD << " = ";
      getUnionValue().printPretty(Out, Ctx, FD->getType());
    }
    Out << '}';
    return;
  case APValue::MemberPointer:
    // FIXME: This is not enough to unambiguously identify the member in a
    // multiple-inheritance scenario.
    if (const ValueDecl *VD = getMemberPointerDecl()) {
      Out << '&' << *cast<CXXRecordDecl>(VD->getDeclContext()) << "::" << *VD;
      return;
    }
    Out << "0";
    return;
  case APValue::AddrLabelDiff:
    Out << "&&" << getAddrLabelDiffLHS()->getLabel()->getName();
    Out << " - ";
    Out << "&&" << getAddrLabelDiffRHS()->getLabel()->getName();
    return;
  }
  llvm_unreachable("Unknown APValue kind!");
}

// This treats string literals and constant char arrays as the same. TODO: Add a
// separate profile function that might be used by code-gen so as not to emit
// multiple global variables of the exact same value.
void APValue::ProfileAsNonTypeTemplateArgument(llvm::FoldingSetNodeID &ID,
                                               const ASTContext &Ctx) const {
  // We need to treat LValues that point to stringliterals as arrays.
  if (getKind() != LValue)
    ID.AddInteger(getKind());
  switch(getKind()) {
  case Uninitialized:
    break;
  case Int:
    getInt().Profile(ID);
    break;
  case Float:
    getFloat().Profile(ID);
    break;
  case Struct: {
    const unsigned NumBases = getStructNumBases();
    for (unsigned I = 0; I != NumBases; ++I)
      getStructBase(I).ProfileAsNonTypeTemplateArgument(ID, Ctx);
    const unsigned NumFields = getStructNumFields();
    for (unsigned I = 0; I != NumFields; ++I)
      getStructField(I).ProfileAsNonTypeTemplateArgument(ID, Ctx);
    break;
  }
  case LValue: {
    struct LValueTemplateArgProfiler : APValueVisitor<LValueTemplateArgProfiler> {
      llvm::FoldingSetNodeID &ID;
      const ASTContext &Ctx;
      LValueTemplateArgProfiler(llvm::FoldingSetNodeID &ID,
                                const ASTContext &Ctx)
          : ID(ID), Ctx(Ctx) {}

      bool VisitNullPtr(APValueParamTy V) {
        // Add the kind
        ID.AddInteger(APValue::LValue);
        ID.AddPointer(nullptr);
        return BaseTy::VisitNullPtr(V);
      }
      bool VisitLValueRootDecl(APValueParamTy V, const ValueDecl *VD) {
        ID.AddInteger(APValue::LValue);
        ID.AddPointer(VD->getCanonicalDecl());
        return BaseTy::VisitLValueRootDecl(V, VD);
      }
      
      bool VisitLValueDeclArraySubObj(APValueParamTy AP,
                                      const ValueDecl *RootDecl,
                                      const ValueDecl *CurArrayDecl,
                                      QualType ElementTy, unsigned ArrayIndex,
                                      unsigned PathIndex, unsigned NumPaths) {
        ID.AddPointer(CurArrayDecl->getCanonicalDecl());
        ID.AddInteger(ArrayIndex);
        return BaseTy::VisitLValueDeclArraySubObj(AP, RootDecl, CurArrayDecl,
                                                  ElementTy, ArrayIndex,
                                                  PathIndex, NumPaths);
      }

      bool VisitLValueDeclBaseSubObj(APValueParamTy AP,
                                     const ValueDecl *RootDecl,
                                     const CXXRecordDecl *ParentClass,
                                     const CXXRecordDecl *BaseClass,
                                     unsigned PathIndex, unsigned NumPaths) {
        ID.AddPointer(BaseClass);
        return BaseTy::VisitLValueDeclBaseSubObj(
            AP, RootDecl, ParentClass, BaseClass, PathIndex, NumPaths);
      }

      bool VisitLValueDeclFieldSubObj(APValueParamTy AP,
                                      const ValueDecl *RootDecl,
                                      const FieldDecl *FD, unsigned PathIndex,
                                      unsigned NumPaths) {
        ID.AddPointer(FD->getCanonicalDecl());
        return BaseTy::VisitLValueDeclFieldSubObj(AP, RootDecl, FD, PathIndex,
                                                  NumPaths);
      }

     
      bool VisitLValueRootExpr(APValueParamTy AP, const Expr *RootExpr) {
        assert(isa<StringLiteral>(RootExpr) && "The only expression that should be a "
                                      "template argument LValue is a string "
                                      "literal!");
        // When profiling a string literal as part of a template argument - we
        // hash it to the same value as an equivalent constant char array.
        // That way:
        // template<char b[4]> struct X;
        // template<> struct X<"abc"> { };
        // template<> struct X<{'a', 'b', 'c'}> { }; --> redefinition
        // 
        ID.AddInteger(Array);
        const StringLiteral *SL = cast<StringLiteral>(RootExpr);

        // get the type of the constant array that was initialized by this
        // string literal.
        //  const char cstr[256] = "abc";

        // The type is char [256] - even though the length of the literal is 3
        // (or is it 4?)
        const ConstantArrayType *StrArrTy =
            cast<ConstantArrayType>(SL->getType().getTypePtr());
        const unsigned ArraySize = Ctx.getConstantArrayElementCount(StrArrTy);
        ID.AddInteger(ArraySize);
        assert(ArraySize >= SL->getLength() &&
               "How is the array size not >= the string literal!");
        QualType CharTy;
        if (SL->isWide())
          CharTy = Ctx.WideCharTy; // L'x' -> wchar_t in C and C++.
        else if (SL->isUTF16())
          CharTy = Ctx.Char16Ty; // u'x' -> char16_t in C11 and C++11.
        else if (SL->isUTF32())
          CharTy = Ctx.Char32Ty; // U'x' -> char32_t in C11 and C++11.
        else
          CharTy = Ctx.CharTy; // 'x' -> char in C++
        unsigned NumInitialized = 0;
        
        for (int N = SL->getLength(); NumInitialized != N; ++NumInitialized) {
          uint32_t CU = SL->getCodeUnit(NumInitialized);
          APValue(Ctx.MakeIntValue(CU, CharTy))
              .ProfileAsNonTypeTemplateArgument(ID, Ctx);
        }
        // Use null code unit fillers to fill in the remaining elements.
        while (NumInitialized++ < ArraySize)
          APValue(Ctx.MakeIntValue(0, CharTy)).ProfileAsNonTypeTemplateArgument(
              ID, Ctx); // Add null code unit fillers.
        
        return BaseTy::VisitLValueRootExpr(AP, RootExpr);
      }

    } LValueTemplateArgProfiler(ID, Ctx);

    LValueTemplateArgProfiler.Visit(*this);
    break;
  }
  case Array: {
    const unsigned ArraySize = getArraySize();
    ID.AddInteger(ArraySize);
    const unsigned NumInitializedElts = getArrayInitializedElts();
    unsigned I;
    for (I = 0; I != NumInitializedElts; ++I)
      getArrayInitializedElt(I).ProfileAsNonTypeTemplateArgument(ID, Ctx);
    // Add the fillers ... so that explicitly and implicitly initialized zeros
    // hash to the same.
    while (I++ != ArraySize) {
      getArrayFiller().ProfileAsNonTypeTemplateArgument(ID, Ctx);
    }
    break;
  }
  case Union:
    ID.AddInteger(getUnionField()->getFieldIndex());
    getUnionValue().ProfileAsNonTypeTemplateArgument(ID, Ctx);
    break;
  case MemberPointer: {
    const ValueDecl * VD = getMemberPointerDecl();
    ID.AddPointer(VD->getCanonicalDecl());
    const bool IsPointerToDerived = isMemberPointerToDerivedMember();
    ID.AddBoolean(IsPointerToDerived);
    ArrayRef<const CXXRecordDecl*> Path = getMemberPointerPath(); 
    for (auto *RD : Path)
      ID.AddPointer(RD->getCanonicalDecl());
    break;
  }
  case ComplexInt:
    llvm_unreachable("Profiling is still unimplemented for APValue::ComplexInt");
  case ComplexFloat:
    llvm_unreachable("Profiling is still unimplemented for APValue::ComplexFloat");
  case Vector:
    llvm_unreachable("Profiling is still unimplemented for APValue::Vector");
  
  case AddrLabelDiff:
    llvm_unreachable("Profiling is still unimplemented for APValue::AddrLabelDiff");
  }
}

std::string APValue::getAsString(ASTContext &Ctx, QualType Ty) const {
  std::string Result;
  llvm::raw_string_ostream Out(Result);
  printPretty(Out, Ctx, Ty);
  Out.flush();
  return Result;
}

const APValue::LValueBase APValue::getLValueBase() const {
  assert(isLValue() && "Invalid accessor");
  return ((const LV*)(const void*)Data.buffer)->BaseAndIsOnePastTheEnd.getPointer();
}

bool APValue::isLValueOnePastTheEnd() const {
  assert(isLValue() && "Invalid accessor");
  return ((const LV*)(const void*)Data.buffer)->BaseAndIsOnePastTheEnd.getInt();
}

CharUnits &APValue::getLValueOffset() {
  assert(isLValue() && "Invalid accessor");
  return ((LV*)(void*)Data.buffer)->Offset;
}

bool APValue::hasLValuePath() const {
  assert(isLValue() && "Invalid accessor");
  return ((const LV*)(const char*)Data.buffer)->hasPath();
}

ArrayRef<APValue::LValuePathEntry> APValue::getLValuePath() const {
  assert(isLValue() && hasLValuePath() && "Invalid accessor");
  const LV &LVal = *((const LV*)(const char*)Data.buffer);
  return llvm::makeArrayRef(LVal.getPath(), LVal.PathLength);
}

unsigned APValue::getLValueCallIndex() const {
  assert(isLValue() && "Invalid accessor");
  return ((const LV*)(const char*)Data.buffer)->CallIndex;
}

void APValue::setLValue(LValueBase B, const CharUnits &O, NoLValuePath,
                        unsigned CallIndex) {
  assert(isLValue() && "Invalid accessor");
  LV &LVal = *((LV*)(char*)Data.buffer);
  LVal.BaseAndIsOnePastTheEnd.setPointer(B);
  LVal.BaseAndIsOnePastTheEnd.setInt(false);
  LVal.Offset = O;
  LVal.CallIndex = CallIndex;
  LVal.resizePath((unsigned)-1);
}

void APValue::setLValue(LValueBase B, const CharUnits &O,
                        ArrayRef<LValuePathEntry> Path, bool IsOnePastTheEnd,
                        unsigned CallIndex) {
  assert(isLValue() && "Invalid accessor");
  LV &LVal = *((LV*)(char*)Data.buffer);
  LVal.BaseAndIsOnePastTheEnd.setPointer(B);
  LVal.BaseAndIsOnePastTheEnd.setInt(IsOnePastTheEnd);
  LVal.Offset = O;
  LVal.CallIndex = CallIndex;
  LVal.resizePath(Path.size());
  memcpy(LVal.getPath(), Path.data(), Path.size() * sizeof(LValuePathEntry));
}

const ValueDecl *APValue::getMemberPointerDecl() const {
  assert(isMemberPointer() && "Invalid accessor");
  const MemberPointerData &MPD =
      *((const MemberPointerData *)(const char *)Data.buffer);
  return MPD.MemberAndIsDerivedMember.getPointer();
}

bool APValue::isMemberPointerToDerivedMember() const {
  assert(isMemberPointer() && "Invalid accessor");
  const MemberPointerData &MPD =
      *((const MemberPointerData *)(const char *)Data.buffer);
  return MPD.MemberAndIsDerivedMember.getInt();
}

ArrayRef<const CXXRecordDecl*> APValue::getMemberPointerPath() const {
  assert(isMemberPointer() && "Invalid accessor");
  const MemberPointerData &MPD =
      *((const MemberPointerData *)(const char *)Data.buffer);
  return llvm::makeArrayRef(MPD.getPath(), MPD.PathLength);
}

void APValue::MakeLValue() {
  assert(isUninit() && "Bad state change");
  static_assert(sizeof(LV) <= DataSize, "LV too big");
  new ((void*)(char*)Data.buffer) LV();
  Kind = LValue;
}

void APValue::MakeArray(unsigned InitElts, unsigned Size) {
  assert(isUninit() && "Bad state change");
  new ((void*)(char*)Data.buffer) Arr(InitElts, Size);
  Kind = Array;
}

void APValue::MakeMemberPointer(const ValueDecl *Member, bool IsDerivedMember,
                                ArrayRef<const CXXRecordDecl*> Path) {
  assert(isUninit() && "Bad state change");
  MemberPointerData *MPD = new ((void*)(char*)Data.buffer) MemberPointerData;
  Kind = MemberPointer;
  MPD->MemberAndIsDerivedMember.setPointer(Member);
  MPD->MemberAndIsDerivedMember.setInt(IsDerivedMember);
  MPD->resizePath(Path.size());
  memcpy(MPD->getPath(), Path.data(), Path.size()*sizeof(const CXXRecordDecl*));
}
