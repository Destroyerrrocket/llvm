//===----- CGOmpSsRuntime.cpp - Interface to OmpSs Runtimes -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This provides a class for OmpSs runtime code generation.
//
//===----------------------------------------------------------------------===//

#include "CGCXXABI.h"
#include "CGCleanup.h"
#include "CGOmpSsRuntime.h"
#include "CGRecordLayout.h"
#include "CodeGenFunction.h"
#include "ConstantEmitter.h"
#include "clang/CodeGen/ConstantInitBuilder.h"
#include "clang/AST/Decl.h"
#include "clang/AST/StmtOmpSs.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Basic/BitmaskEnum.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "llvm/IR/Intrinsics.h"

using namespace clang;
using namespace CodeGen;

namespace {
class OSSDependVisitor
  : public ConstStmtVisitor<OSSDependVisitor, void> {
  CodeGenFunction &CGF;
  bool OSSSyntax;

  llvm::Type *OSSArgTy;

  llvm::Value *Ptr;
  SmallVector<llvm::Value *, 4> Starts;
  SmallVector<llvm::Value *, 4> Ends;
  SmallVector<llvm::Value *, 4> Dims;
  QualType BaseElementTy;

public:

  OSSDependVisitor(CodeGenFunction &CGF, bool OSSSyntax)
    : CGF(CGF), OSSSyntax(OSSSyntax),
      OSSArgTy(CGF.ConvertType(CGF.getContext().LongTy))
      {}

  //===--------------------------------------------------------------------===//
  //                               Utilities
  //===--------------------------------------------------------------------===//

  QualType GetInnermostElementType(const QualType &Q) {
    if (Q->isArrayType()) {
      if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(Q)) {
        return CGF.getContext().getBaseElementType(Q);
      } else if (const VariableArrayType *BaseArrayTy = CGF.getContext().getAsVariableArrayType(Q)) {
        return CGF.getContext().getBaseElementType(Q);
      } else {
        llvm_unreachable("Unhandled array type");
      }
    }
    return Q;
  }

  void FillBaseExprDimsAndType(const Expr *E) {
    BaseElementTy = GetInnermostElementType(E->getType());
    QualType TmpTy = E->getType();
    // Add Dimensions
    if (TmpTy->isPointerType() || !TmpTy->isArrayType()) {
      // T * || T
      Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
    }
    while (TmpTy->isArrayType()) {
      // T []
      if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(TmpTy)) {
        uint64_t DimSize = BaseArrayTy->getSize().getSExtValue();
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, DimSize));
        TmpTy = BaseArrayTy->getElementType();
      } else if (const VariableArrayType *BaseArrayTy = CGF.getContext().getAsVariableArrayType(TmpTy)) {
        auto VlaSize = CGF.getVLAElements1D(BaseArrayTy);
        llvm::Value *DimExpr = CGF.Builder.CreateSExt(VlaSize.NumElts, OSSArgTy);
        Dims.push_back(DimExpr);
        TmpTy = BaseArrayTy->getElementType();
      } else {
        llvm_unreachable("Unhandled array type");
      }
    }
  }

  // This is used in the innermost Expr * in ArraySubscripts and OSSArraySection
  void FillDimsFromInnermostExpr(const Expr *E) {
    // Go through the expression which may be a DeclRefExpr or MemberExpr or OSSArrayShapingExpr
    E = E->IgnoreParenImpCasts();
    QualType TmpTy = E->getType();
    // Add Dimensions
    if (TmpTy->isPointerType()) {
      // T *
      // We have added section dimension before
      if (Dims.empty())
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
      TmpTy = TmpTy->getPointeeType();
    }
    while (TmpTy->isArrayType()) {
      // T []
      if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(TmpTy)) {
        uint64_t DimSize = BaseArrayTy->getSize().getSExtValue();
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, DimSize));
        TmpTy = BaseArrayTy->getElementType();
      } else if (const VariableArrayType *BaseArrayTy = CGF.getContext().getAsVariableArrayType(TmpTy)) {
        auto VlaSize = CGF.getVLAElements1D(BaseArrayTy);
        llvm::Value *DimExpr = CGF.Builder.CreateSExt(VlaSize.NumElts, OSSArgTy);
        Dims.push_back(DimExpr);
        TmpTy = BaseArrayTy->getElementType();
      } else {
        llvm_unreachable("Unhandled array type");
      }
    }
  }

  //===--------------------------------------------------------------------===//
  //                            Visitor Methods
  //===--------------------------------------------------------------------===//

  void Visit(const Expr *E) {
    ConstStmtVisitor<OSSDependVisitor, void>::Visit(E);
  }

  void VisitStmt(const Stmt *S) {
    llvm_unreachable("Unhandled stmt");
  }

  void VisitExpr(const Expr *E) {
    llvm_unreachable("Unhandled expr");
  }

  void VisitOSSArrayShapingExpr(const OSSArrayShapingExpr *E) {
    BaseElementTy = GetInnermostElementType(E->getType());
    Ptr = CGF.EmitLValue(E).getPointer();
    if (E->getType()->isVariablyModifiedType()) {
      CGF.EmitVariablyModifiedType(E->getType());
    }
    FillDimsFromInnermostExpr(E);
  }

  // l-values.
  void VisitDeclRefExpr(const DeclRefExpr *E) {
    Ptr = CGF.EmitDeclRefLValue(E).getPointer();
    FillBaseExprDimsAndType(E);
  }

  void VisitOSSArraySectionExpr(const OSSArraySectionExpr *E) {
    // Get Base Type
    // An array section is considered a built-in type
    BaseElementTy =
        OSSArraySectionExpr::getBaseOriginalType(
                          E->getBase());
    if (BaseElementTy->isAnyPointerType()) {
      BaseElementTy = BaseElementTy->getPointeeType();
    } else if (BaseElementTy->isArrayType()) {
      BaseElementTy = BaseElementTy->getAsArrayTypeUnsafe()->getElementType();
    } else {
      llvm_unreachable("Unhandled Type");
    }
    BaseElementTy = GetInnermostElementType(BaseElementTy);

    // Get the inner expr
    const Expr *TmpE = E;
    // First come OSSArraySection
    while (const OSSArraySectionExpr *ASE = dyn_cast<OSSArraySectionExpr>(TmpE->IgnoreParenImpCasts())) {
      // Stop in the innermost ArrayToPointerDecay
      TmpE = ASE->getBase();
      // Add indexes
      llvm::Value *Idx, *IdxEnd;

      const Expr *LowerB = ASE->getLowerBound();
      if (LowerB)
        Idx = CGF.EmitScalarExpr(LowerB);
      else
        // OpenMP 5.0 2.1.5 When the lower-bound is absent it defaults to 0.
        Idx = llvm::ConstantInt::getSigned(OSSArgTy, 0);
      Idx = CGF.Builder.CreateSExt(Idx, OSSArgTy);

      const Expr *LengthUpper = ASE->getLengthUpper();
      bool ColonForm = ASE->isColonForm();
      if (LengthUpper &&
          (!OSSSyntax || (OSSSyntax && !ColonForm))) {
        // depend(in: array[ : length])
        // in(array[ ; length])
        IdxEnd = CGF.EmitScalarExpr(LengthUpper);
        IdxEnd = CGF.Builder.CreateSExt(IdxEnd, OSSArgTy);
        IdxEnd = CGF.Builder.CreateAdd(Idx, IdxEnd);
      } else if (LengthUpper
                 && (OSSSyntax && ColonForm)) {
        // in(array[ : upper])
        IdxEnd = CGF.EmitScalarExpr(LengthUpper);
        IdxEnd = CGF.Builder.CreateSExt(IdxEnd, OSSArgTy);
        IdxEnd = CGF.Builder.CreateAdd(llvm::ConstantInt::getSigned(OSSArgTy, 1), IdxEnd);
      } else if (ASE->getColonLoc().isInvalid()) {
        assert(!LengthUpper);
        // OSSArraySection without ':' are regular array subscripts
        IdxEnd = CGF.Builder.CreateAdd(Idx, llvm::ConstantInt::getSigned(OSSArgTy, 1));
      } else {

        // OpenMP 5.0 2.1.5
        // depend(in: array[lower : ]) -> [lower, dimsize)
        // When the length is absent it defaults to ⌈(size - lowerbound)∕stride⌉,
        // where size is the size of the array dimension.
        //
        // OmpSs-2
        // in(array[lower ; ]) -> [lower, dimsize)
        // in(array[lower : ]) -> [lower, dimsize)
        QualType BaseOriginalTy =
          OSSArraySectionExpr::getBaseOriginalType(ASE->getBase());

        if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(BaseOriginalTy)) {
          uint64_t DimSize = BaseArrayTy->getSize().getSExtValue();
          IdxEnd = llvm::ConstantInt::getSigned(OSSArgTy, DimSize);
        } else if (const VariableArrayType *BaseArrayTy = CGF.getContext().getAsVariableArrayType(BaseOriginalTy)) {
          auto VlaSize = CGF.getVLAElements1D(BaseArrayTy);
          IdxEnd = CGF.Builder.CreateSExt(VlaSize.NumElts, OSSArgTy);
        } else {
          llvm_unreachable("Unhandled array type");
        }
      }
      IdxEnd = CGF.Builder.CreateSExt(IdxEnd, OSSArgTy);

      Starts.push_back(Idx);
      Ends.push_back(IdxEnd);
      // If we see a Pointer we must to add one dimension and done
      if (TmpE->IgnoreParenImpCasts()->getType()->isPointerType()) {
        assert(LengthUpper && "Sema should have forbidden unspecified sizes in pointers");
        Dims.push_back(CGF.Builder.CreateSExt(CGF.EmitScalarExpr(LengthUpper), OSSArgTy));
        break;
      }
    }
    while (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(TmpE->IgnoreParenImpCasts())) {
      // Stop in the innermost ArrayToPointerDecay
      TmpE = ASE->getBase();
      // Add indexes
      llvm::Value *Idx = CGF.EmitScalarExpr(ASE->getIdx());
      Idx = CGF.Builder.CreateSExt(Idx, OSSArgTy);
      llvm::Value *IdxEnd = CGF.Builder.CreateAdd(Idx, llvm::ConstantInt::getSigned(OSSArgTy, 1));
      Starts.push_back(Idx);
      Ends.push_back(IdxEnd);
      // If we see a Pointer we must to add one dimension and done
      if (TmpE->IgnoreParenImpCasts()->getType()->isPointerType()) {
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
        break;
      }
    }

    Ptr = CGF.EmitScalarExpr(TmpE);
    if (const OSSArrayShapingExpr *OSA = dyn_cast<OSSArrayShapingExpr>(TmpE->IgnoreParenImpCasts())) {
      // We must to emit VLA args
      if (OSA->getType()->isVariablyModifiedType()) {
        CGF.EmitVariablyModifiedType(OSA->getType());
      }
    }
    FillDimsFromInnermostExpr(TmpE);
  }

  void VisitArraySubscriptExpr(const ArraySubscriptExpr *E) {
    // Get Base Type
    BaseElementTy = GetInnermostElementType(E->getType());
    // Get the inner expr
    const Expr *TmpE = E;
    while (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(TmpE->IgnoreParenImpCasts())) {
      // Stop in the innermost ArrayToPointerDecay
      TmpE = ASE->getBase();
      // Add indexes
      llvm::Value *Idx = CGF.EmitScalarExpr(ASE->getIdx());
      Idx = CGF.Builder.CreateSExt(Idx, OSSArgTy);
      llvm::Value *IdxEnd = CGF.Builder.CreateAdd(Idx, llvm::ConstantInt::getSigned(OSSArgTy, 1));
      Starts.push_back(Idx);
      Ends.push_back(IdxEnd);
      // If we see a Pointer we must to add one dimension and done
      if (TmpE->IgnoreParenImpCasts()->getType()->isPointerType()) {
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
        break;
      }
    }

    Ptr = CGF.EmitScalarExpr(TmpE);
    if (const OSSArrayShapingExpr *OSA = dyn_cast<OSSArrayShapingExpr>(TmpE->IgnoreParenImpCasts())) {
      // We must to emit VLA args
      if (OSA->getType()->isVariablyModifiedType()) {
        CGF.EmitVariablyModifiedType(OSA->getType());
      }
    }
    FillDimsFromInnermostExpr(TmpE);
  }

  void VisitMemberExpr(const MemberExpr *E) {
    Ptr = CGF.EmitMemberExpr(E).getPointer();
    FillBaseExprDimsAndType(E);
  }

  void VisitUnaryDeref(const UnaryOperator *E) {
    Ptr = CGF.EmitUnaryOpLValue(E).getPointer();
    FillBaseExprDimsAndType(E);
  }

  ArrayRef<llvm::Value *> getStarts() const {
    return Starts;
  }

  ArrayRef<llvm::Value *> getEnds() const {
    return Ends;
  }

  ArrayRef<llvm::Value *> getDims() const {
    return Dims;
  }

  QualType getBaseElementTy() {
    return BaseElementTy;
  }

  llvm::Value *getPtr() {
    return Ptr;
  }

};
} // namespace

static LValue EmitRefAsIs(CodeGenFunction &CGF, const VarDecl *VD) {
  Address addr = Address::invalid();
  //                                hasLocalStorage()     hasLinkage()
  // (global) int &rx;                     0                  1
  // struct { static int &rx; };           0                  1
  // int main() { static int &rx; }        0                  0
  if (!VD->hasLocalStorage()) {
    llvm::Value *V = CGF.CGM.GetAddrOfGlobalVar(VD);
    CharUnits Alignment = CGF.getContext().getDeclAlign(VD);
    addr = Address(V, Alignment);
  } else {
    addr = CGF.GetAddrOfLocalVar(VD);
  }

  return CGF.MakeAddrLValue(addr, VD->getType(), AlignmentSource::Decl);
}

static void EmitVLADims(CodeGenFunction &CGF, llvm::Value *V, QualType Q,
                        SmallVectorImpl<llvm::OperandBundleDef> &TaskInfo,
                        SmallVectorImpl<llvm::Value *> &CapturedList) {
  // C long -> LLVM long
  llvm::Type *OSSArgTy = CGF.ConvertType(CGF.getContext().LongTy);

  SmallVector<llvm::Value*, 4> DimsWithValue;
  DimsWithValue.push_back(V);
  while (Q->isArrayType()) {
    if (const VariableArrayType *BaseArrayTy = CGF.getContext().getAsVariableArrayType(Q)) {
      auto VlaSize = CGF.getVLAElements1D(BaseArrayTy);
      llvm::Value *DimExpr = CGF.Builder.CreateSExt(VlaSize.NumElts, OSSArgTy);
      DimsWithValue.push_back(DimExpr);
      CapturedList.push_back(DimExpr);
      Q = BaseArrayTy->getElementType();
    } else if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(Q)) {
      uint64_t DimSize = BaseArrayTy->getSize().getSExtValue();
      llvm::Value *DimConstant = llvm::ConstantInt::getSigned(OSSArgTy, DimSize);
      DimsWithValue.push_back(DimConstant);
      CapturedList.push_back(DimConstant);
      Q = BaseArrayTy->getElementType();
    } else {
      llvm_unreachable("Unhandled array type");
    }
  }
  assert(DimsWithValue.size() >= 1);
  TaskInfo.emplace_back("QUAL.OSS.VLA.DIMS", DimsWithValue);
}

static void EmitDSA(StringRef Name, CodeGenFunction &CGF, const Expr *E,
                    SmallVectorImpl<llvm::OperandBundleDef> &TaskInfo,
                    SmallVectorImpl<llvm::Value*> &CapturedList) {
  std::string Basename = Name;

  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
    const VarDecl *VD = cast<VarDecl>(DRE->getDecl());
    llvm::Value *V;
    if (VD->getType()->isReferenceType()) {
      // Emit the reference Value as is since EmitDeclRefLValue will emit a load of it
      V = EmitRefAsIs(CGF, VD).getPointer();
      TaskInfo.emplace_back(Basename, V);
    } else {
      V = CGF.EmitDeclRefLValue(DRE).getPointer();
      TaskInfo.emplace_back(Basename, V);
    }
    QualType Q = VD->getType();
    if (Q->isVariableArrayType())
      EmitVLADims(CGF, V, Q, TaskInfo, CapturedList);
  } else if (const CXXThisExpr *ThisE = dyn_cast<CXXThisExpr>(E)) {
    TaskInfo.emplace_back(Basename, CGF.EmitScalarExpr(ThisE));
  } else {
    llvm_unreachable("Unhandled expression");
  }
}

static void EmitDependency(StringRef Name, CodeGenFunction &CGF, const OSSDepDataTy &Dep,
                           SmallVectorImpl<llvm::OperandBundleDef> &TaskInfo) {

  // C long -> LLVM long
  llvm::Type *OSSArgTy = CGF.ConvertType(CGF.getContext().LongTy);

  OSSDependVisitor DepVisitor(CGF, Dep.OSSSyntax);
  DepVisitor.Visit(Dep.E);

  SmallVector<llvm::Value*, 4> DepData;

  SmallVector<llvm::Value *, 4> Starts(
      DepVisitor.getStarts().begin(),
      DepVisitor.getStarts().end());

  SmallVector<llvm::Value *, 4> Ends(
      DepVisitor.getEnds().begin(),
      DepVisitor.getEnds().end());

  SmallVector<llvm::Value *, 4> Dims(
      DepVisitor.getDims().begin(),
      DepVisitor.getDims().end());
  QualType BaseElementTy = DepVisitor.getBaseElementTy();
  llvm::Value *Ptr = DepVisitor.getPtr();

  uint64_t BaseElementSize =
             CGF.CGM
               .getDataLayout()
               .getTypeSizeInBits(CGF
                                  .ConvertType(BaseElementTy))/8;

  DepData.push_back(Ptr);
  bool First = true;
  for (size_t i = Dims.size() - 1; i > 0; --i) {
    llvm::Value *Dim = Dims[i];
    llvm::Value *IdxStart;
    llvm::Value *IdxEnd;
    // In arrays we have to output all dimensions, but
    // the number of indices may be less than the number
    // of dimensions (array[1] -> int array[10][20])
    if (i < Starts.size()) {
      IdxStart = Starts[Starts.size() - i - 1];
      IdxEnd = Ends[Starts.size() - i - 1];
    } else {
      IdxStart = llvm::ConstantInt::getSigned(OSSArgTy, 0);
      IdxEnd = Dim;
    }
    if (First) {
      First = false;
      Dim = CGF.Builder.CreateMul(Dim,
                                  llvm::ConstantInt::getSigned(OSSArgTy,
                                                               BaseElementSize));
      IdxStart = CGF.Builder.CreateMul(IdxStart,
                                  llvm::ConstantInt::getSigned(OSSArgTy,
                                                               BaseElementSize));
      IdxEnd = CGF.Builder.CreateMul(IdxEnd,
                                  llvm::ConstantInt::getSigned(OSSArgTy,
                                                               BaseElementSize));
    }
    DepData.push_back(Dim);
    DepData.push_back(IdxStart);
    DepData.push_back(IdxEnd);
  }
  llvm::Value *Dim = Dims[0];
  llvm::Value *IdxStart;
  llvm::Value *IdxEnd;
  if (Starts.size() > 0) {
    IdxStart = Starts[Starts.size() - 1];
    IdxEnd = Ends[Starts.size() - 1];
  } else {
    IdxStart = llvm::ConstantInt::getSigned(OSSArgTy, 0);
    IdxEnd = Dim;
  }

  if (First) {
    First = false;
    Dim = CGF.Builder.CreateMul(Dim,
                                llvm::ConstantInt::getSigned(OSSArgTy,
                                                             BaseElementSize));
    IdxStart = CGF.Builder.CreateMul(IdxStart,
                                llvm::ConstantInt::getSigned(OSSArgTy,
                                                             BaseElementSize));
    IdxEnd = CGF.Builder.CreateMul(IdxEnd,
                                llvm::ConstantInt::getSigned(OSSArgTy,
                                                             BaseElementSize));
  }
  DepData.push_back(Dim);
  DepData.push_back(IdxStart);
  DepData.push_back(IdxEnd);

  TaskInfo.emplace_back(Name, makeArrayRef(DepData));
}

void CGOmpSsRuntime::emitTaskwaitCall(CodeGenFunction &CGF,
                                      SourceLocation Loc) {
  llvm::Value *Callee = CGM.getIntrinsic(llvm::Intrinsic::directive_marker);
  CGF.Builder.CreateCall(Callee,
                         {},
                         {llvm::OperandBundleDef("DIR.OSS",
                                                 llvm::ConstantDataArray::getString(CGM.getLLVMContext(),
                                                                                    "TASKWAIT"))});
}

bool CGOmpSsRuntime::inTask() {
  return !TaskEntryStack.empty() || InTaskEntryEmission;
}

llvm::AssertingVH<llvm::Instruction> CGOmpSsRuntime::getCurrentTask() {
  return TaskEntryStack.back();
}

void CGOmpSsRuntime::emitTaskCall(CodeGenFunction &CGF,
                                  const OSSExecutableDirective &D,
                                  SourceLocation Loc,
                                  const OSSTaskDataTy &Data) {
  InTaskEntryEmission = true;

  llvm::Value *EntryCallee = CGM.getIntrinsic(llvm::Intrinsic::directive_region_entry);
  llvm::Value *ExitCallee = CGM.getIntrinsic(llvm::Intrinsic::directive_region_exit);
  SmallVector<llvm::OperandBundleDef, 8> TaskInfo;
  TaskInfo.emplace_back("DIR.OSS", llvm::ConstantDataArray::getString(CGM.getLLVMContext(), "TASK"));

  SmallVector<llvm::Value*, 4> CapturedList;
  for (const Expr *E : Data.DSAs.Shareds) {
    EmitDSA("QUAL.OSS.SHARED", CGF, E, TaskInfo, CapturedList);
  }
  for (const Expr *E : Data.DSAs.Privates) {
    EmitDSA("QUAL.OSS.PRIVATE", CGF, E, TaskInfo, CapturedList);
  }
  for (const Expr *E : Data.DSAs.Firstprivates) {
    EmitDSA("QUAL.OSS.FIRSTPRIVATE", CGF, E, TaskInfo, CapturedList);
  }

  if (!CapturedList.empty())
    TaskInfo.emplace_back("QUAL.OSS.CAPTURED", CapturedList);

  for (const OSSDepDataTy &Dep : Data.Deps.Ins) {
    EmitDependency("QUAL.OSS.DEP.IN", CGF, Dep, TaskInfo);
  }
  for (const OSSDepDataTy &Dep : Data.Deps.Outs) {
    EmitDependency("QUAL.OSS.DEP.OUT", CGF, Dep, TaskInfo);
  }
  for (const OSSDepDataTy &Dep : Data.Deps.Inouts) {
    EmitDependency("QUAL.OSS.DEP.INOUT", CGF, Dep, TaskInfo);
  }
  for (const OSSDepDataTy &Dep : Data.Deps.WeakIns) {
    EmitDependency("QUAL.OSS.DEP.WEAKIN", CGF, Dep, TaskInfo);
  }
  for (const OSSDepDataTy &Dep : Data.Deps.WeakOuts) {
    EmitDependency("QUAL.OSS.DEP.WEAKOUT", CGF, Dep, TaskInfo);
  }
  for (const OSSDepDataTy &Dep : Data.Deps.WeakInouts) {
    EmitDependency("QUAL.OSS.DEP.WEAKINOUT", CGF, Dep, TaskInfo);
  }
  if (Data.If)
    TaskInfo.emplace_back("QUAL.OSS.IF", CGF.EvaluateExprAsBool(Data.If));
  if (Data.Final)
    TaskInfo.emplace_back("QUAL.OSS.FINAL", CGF.EvaluateExprAsBool(Data.Final));


  llvm::Instruction *Result =
    CGF.Builder.CreateCall(EntryCallee, {}, llvm::makeArrayRef(TaskInfo));

  InTaskEntryEmission = false;

  // Push Task Stack
  llvm::Value *Undef = llvm::UndefValue::get(CGF.Int32Ty);
  llvm::Instruction *TaskAllocaInsertPt = new llvm::BitCastInst(Undef, CGF.Int32Ty, "taskallocapt", Result->getParent());
  TaskEntryStack.push_back(TaskAllocaInsertPt);

  CGF.EmitStmt(D.getAssociatedStmt());

  CGF.Builder.CreateCall(ExitCallee, Result);

  // Pop Task Stack
  TaskEntryStack.pop_back();
  TaskAllocaInsertPt->eraseFromParent();

}

