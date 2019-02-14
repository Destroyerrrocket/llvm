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

class OSSDependVisitor
  : public ConstStmtVisitor<OSSDependVisitor, void> {
  CodeGenFunction &CGF;

  llvm::Type *OSSArgTy;

  llvm::Value *Ptr;
  SmallVector<llvm::Value *, 4> Starts;
  SmallVector<llvm::Value *, 4> Ends;
  SmallVector<llvm::Value *, 4> Dims;
  QualType BaseElementTy;

public:

  OSSDependVisitor(CodeGenFunction &CGF)
    : CGF(CGF),
      OSSArgTy(CGF.ConvertType(CGF.getContext().LongTy))
      {}

  //===--------------------------------------------------------------------===//
  //                               Utilities
  //===--------------------------------------------------------------------===//

  void FillBaseExprDimsAndType(const Expr *E) {
    BaseElementTy = E->getType();
    if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(BaseElementTy)){
      BaseElementTy = CGF.getContext().getBaseElementType(BaseElementTy);
    }
    QualType TmpTy = E->getType();
    // Add Dimensions
    if (TmpTy->isPointerType()) {
      // T *
      Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
    } else if (!TmpTy->isArrayType()) {
      // T
      Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
    }
    while (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(TmpTy)){
      // T []
      uint64_t DimSize = BaseArrayTy->getSize().getSExtValue();
      Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, DimSize));
      TmpTy = BaseArrayTy->getElementType();
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

  // l-values.
  void VisitDeclRefExpr(const DeclRefExpr *E) {
    Ptr = CGF.EmitDeclRefLValue(E).getPointer();
    FillBaseExprDimsAndType(E);
  }

  void VisitArraySubscriptExpr(const ArraySubscriptExpr *E) {
    BaseElementTy = E->getType();
    // Get Base Type
    if (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(BaseElementTy)){
      BaseElementTy = CGF.getContext().getBaseElementType(BaseElementTy);
    }
    // Get the inner expr
    const Expr *Expr = E;
    while (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(Expr->IgnoreParenImpCasts())) {
      Expr = ASE->getBase();
      // Add indexes
      llvm::Value *Idx = ConstantEmitter(CGF).emitAbstract(ASE->getIdx(),
                                                           ASE->getIdx()->getType());
      Idx = CGF.Builder.CreateSExt(Idx, OSSArgTy);
      llvm::Value *IdxEnd = CGF.Builder.CreateAdd(Idx, llvm::ConstantInt::getSigned(OSSArgTy, 1));
      Starts.push_back(Idx);
      Ends.push_back(IdxEnd);
      if (Expr->IgnoreParenImpCasts()->getType()->isPointerType())
        break;
    }

    Ptr = CGF.EmitScalarExpr(Expr);

    if (const CastExpr *CE = dyn_cast<CastExpr>(Expr)) {
      QualType TmpTy = CE->getType();
      // Add Dimensions
      if (TmpTy->isPointerType()) {
        // T (*)[]
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
        TmpTy = cast<PointerType>(TmpTy)->getPointeeType();
        while (const ConstantArrayType *BaseArrayTy = CGF.getContext().getAsConstantArrayType(TmpTy)){
          // T []
          uint64_t DimSize = BaseArrayTy->getSize().getSExtValue();
          Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, DimSize));
          TmpTy = BaseArrayTy->getElementType();
        }
      } else {
        // T *
        Dims.push_back(llvm::ConstantInt::getSigned(OSSArgTy, 1));
      }
    }
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

static void EmitDSA(StringRef Name, CodeGenFunction &CGF, const Expr *E,
                    SmallVectorImpl<llvm::OperandBundleDef> &TaskInfo) {
  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
    TaskInfo.emplace_back(Name, CGF.EmitDeclRefLValue(DRE).getPointer());
  }
  else {
    llvm_unreachable("Unhandled expression");
  }
}

static void EmitDependency(StringRef Name, CodeGenFunction &CGF, const Expr *E,
                           SmallVectorImpl<llvm::OperandBundleDef> &TaskInfo) {

  // C long -> LLVM long
  llvm::Type *OSSArgTy = CGF.ConvertType(CGF.getContext().LongTy);

  OSSDependVisitor DepVisitor(CGF);
  DepVisitor.Visit(E);

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

void CGOmpSsRuntime::emitTaskCall(CodeGenFunction &CGF,
                                  const OSSExecutableDirective &D,
                                  SourceLocation Loc,
                                  const OSSTaskDataTy &Data) {
  llvm::Value *EntryCallee = CGM.getIntrinsic(llvm::Intrinsic::directive_region_entry);
  llvm::Value *ExitCallee = CGM.getIntrinsic(llvm::Intrinsic::directive_region_exit);
  SmallVector<llvm::OperandBundleDef, 8> TaskInfo;
  TaskInfo.emplace_back("DIR.OSS", llvm::ConstantDataArray::getString(CGM.getLLVMContext(), "TASK"));
  for (const Expr *E : Data.SharedVars) {
    EmitDSA("QUAL.OSS.SHARED", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.PrivateVars) {
    EmitDSA("QUAL.OSS.PRIVATE", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.FirstprivateVars) {
    EmitDSA("QUAL.OSS.FIRSTPRIVATE", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.DependIn) {
    EmitDependency("QUAL.OSS.DEP.IN", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.DependOut) {
    EmitDependency("QUAL.OSS.DEP.OUT", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.DependInout) {
    EmitDependency("QUAL.OSS.DEP.INOUT", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.DependWeakIn) {
    EmitDependency("QUAL.OSS.DEP.WEAKIN", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.DependWeakOut) {
    EmitDependency("QUAL.OSS.DEP.WEAKOUT", CGF, E, TaskInfo);
  }
  for (const Expr *E : Data.DependWeakInout) {
    EmitDependency("QUAL.OSS.DEP.WEAKINOUT", CGF, E, TaskInfo);
  }

  llvm::Value *Result =
    CGF.Builder.CreateCall(EntryCallee,
                           {},
                           llvm::makeArrayRef(TaskInfo));

  CGF.EmitStmt(D.getAssociatedStmt());

  CGF.Builder.CreateCall(ExitCallee,
                         Result);
}

