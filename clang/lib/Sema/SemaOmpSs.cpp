//===--- SemaOmpSs.cpp - Semantic Analysis for OmpSs constructs ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements semantic analysis for OmpSs directives and
/// clauses.
///
//===----------------------------------------------------------------------===//

#include "TreeTransform.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTMutationListener.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtOmpSs.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Basic/OmpSsKinds.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
#include "llvm/ADT/PointerEmbeddedInt.h"
using namespace clang;

namespace {
/// Default data sharing attributes, which can be applied to directive.
enum DefaultDataSharingAttributes {
  DSA_unspecified = 0, /// Data sharing attribute not specified.
  DSA_none = 1 << 0,   /// Default data sharing attribute 'none'.
  DSA_shared = 1 << 1, /// Default data sharing attribute 'shared'.
};

/// Stack for tracking declarations used in OmpSs directives and
/// clauses and their data-sharing attributes.
class DSAStackTy {
public:
  struct DSAVarData {
    OmpSsDirectiveKind DKind = OSSD_unknown;
    OmpSsClauseKind CKind = OSSC_unknown;
    const Expr *RefExpr = nullptr;
    bool IsImplicit = true;
    DSAVarData() = default;
    DSAVarData(OmpSsDirectiveKind DKind, OmpSsClauseKind CKind,
               const Expr *RefExpr, bool IsImplicit)
        : DKind(DKind), CKind(CKind), RefExpr(RefExpr), IsImplicit(IsImplicit)
          {}
  };

private:
  struct DSAInfo {
    OmpSsClauseKind Attributes = OSSC_unknown;
    const Expr * RefExpr;
    bool IsImplicit;
  };
  using DeclSAMapTy = llvm::SmallDenseMap<const ValueDecl *, DSAInfo, 8>;

  // Directive
  struct SharingMapTy {
    DeclSAMapTy SharingMap;
    DefaultDataSharingAttributes DefaultAttr = DSA_unspecified;
    OmpSsDirectiveKind Directive = OSSD_unknown;
    Scope *CurScope = nullptr;
    SourceLocation ConstructLoc;
    SharingMapTy(OmpSsDirectiveKind DKind,
                 Scope *CurScope, SourceLocation Loc)
        : Directive(DKind), CurScope(CurScope),
          ConstructLoc(Loc) {}
    SharingMapTy() = default;
  };

  using StackTy = SmallVector<SharingMapTy, 4>;

  /// Stack of used declaration and their data-sharing attributes.
  const FunctionScopeInfo *CurrentNonCapturingFunctionScope = nullptr;
  SmallVector<std::pair<StackTy, const FunctionScopeInfo *>, 4> Stack;
  Sema &SemaRef;

  using iterator = StackTy::const_reverse_iterator;

  DSAVarData getDSA(iterator &Iter, ValueDecl *D) const;

  bool isStackEmpty() const {
    return Stack.empty() ||
           Stack.back().second != CurrentNonCapturingFunctionScope ||
           Stack.back().first.empty();
  }

public:
  explicit DSAStackTy(Sema &S) : SemaRef(S) {}

  void push(OmpSsDirectiveKind DKind,
            Scope *CurScope, SourceLocation Loc) {
    if (Stack.empty() ||
        Stack.back().second != CurrentNonCapturingFunctionScope)
      Stack.emplace_back(StackTy(), CurrentNonCapturingFunctionScope);
    Stack.back().first.emplace_back(DKind, CurScope, Loc);
  }

  void pop() {
    assert(!Stack.back().first.empty() &&
           "Data-sharing attributes stack is empty!");
    Stack.back().first.pop_back();
  }

  /// Adds explicit data sharing attribute to the specified declaration.
  void addDSA(const ValueDecl *D, const Expr *E, OmpSsClauseKind A, bool IsImplicit);

  /// Returns data sharing attributes from top of the stack for the
  /// specified declaration.
  const DSAVarData getTopDSA(ValueDecl *D, bool FromParent);
  /// Returns data sharing attributes from the current directive for the
  /// specified declaration.
  const DSAVarData getCurrentDSA(ValueDecl *D);
  /// Returns data-sharing attributes for the specified declaration.
  /// Checks if the specified variables has data-sharing attributes which
  /// match specified \a CPred predicate in any directive which matches \a DPred
  /// predicate.
  const DSAVarData
  hasDSA(ValueDecl *D, const llvm::function_ref<bool(OmpSsClauseKind)> CPred,
         const llvm::function_ref<bool(OmpSsDirectiveKind)> DPred,
         bool FromParent) const;
  /// Returns currently analyzed directive.
  OmpSsDirectiveKind getCurrentDirective() const {
    return isStackEmpty() ? OSSD_unknown : Stack.back().first.back().Directive;
  }
};

} // namespace

static const ValueDecl *getCanonicalDecl(const ValueDecl *D) {
  const auto *VD = dyn_cast<VarDecl>(D);
  const auto *FD = dyn_cast<FieldDecl>(D);
  if (VD != nullptr) {
    VD = VD->getCanonicalDecl();
    D = VD;
  } else {
    assert(FD);
    FD = FD->getCanonicalDecl();
    D = FD;
  }
  return D;
}

static ValueDecl *getCanonicalDecl(ValueDecl *D) {
  return const_cast<ValueDecl *>(
      getCanonicalDecl(const_cast<const ValueDecl *>(D)));
}

DSAStackTy::DSAVarData DSAStackTy::getDSA(iterator &Iter,
                                          ValueDecl *D) const {
  D = getCanonicalDecl(D);
  DSAVarData DVar;

  DVar.DKind = Iter->Directive;
  if (Iter->SharingMap.count(D)) {
    const DSAInfo &Data = Iter->SharingMap.lookup(D);
    DVar.RefExpr = Data.RefExpr;
    DVar.IsImplicit = Data.IsImplicit;
    DVar.CKind = Data.Attributes;
    return DVar;
  }

  return DVar;
}

void DSAStackTy::addDSA(const ValueDecl *D, const Expr *E, OmpSsClauseKind A, bool IsImplicit) {
  D = getCanonicalDecl(D);
  assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
  DSAInfo &Data = Stack.back().first.back().SharingMap[D];
  Data.Attributes = A;
  Data.RefExpr = E;
  Data.IsImplicit = IsImplicit;
}

const DSAStackTy::DSAVarData DSAStackTy::getTopDSA(ValueDecl *D,
                                                   bool FromParent) {
  D = getCanonicalDecl(D);
  DSAVarData DVar;

  auto *VD = dyn_cast<VarDecl>(D);

  auto &&IsTaskDir = [](OmpSsDirectiveKind Dir) { return Dir == OSSD_task; };
  auto &&AnyClause = [](OmpSsClauseKind Clause) { return Clause != OSSC_shared; };
  if (VD) {
    DSAVarData DVarTemp = hasDSA(D, AnyClause, IsTaskDir, FromParent);
    if (DVarTemp.CKind != OSSC_unknown && DVarTemp.RefExpr)
      return DVarTemp;
  }

  return DVar;
}

const DSAStackTy::DSAVarData DSAStackTy::getCurrentDSA(ValueDecl *D) {
  D = getCanonicalDecl(D);
  DSAVarData DVar;

  auto *VD = dyn_cast<VarDecl>(D);

  auto &&IsTaskDir = [](OmpSsDirectiveKind Dir) { return Dir == OSSD_task; };
  auto &&AnyClause = [](OmpSsClauseKind Clause) { return true; };
  iterator I = Stack.back().first.rbegin();
  iterator EndI = Stack.back().first.rend();
  if (VD){
    if (I != EndI) {
      if (IsTaskDir(I->Directive)) {
        DSAVarData DVar = getDSA(I, D);
          if (AnyClause(DVar.CKind))
            return DVar;
      }
    }
  }
  return DVar;
}

const DSAStackTy::DSAVarData
DSAStackTy::hasDSA(ValueDecl *D,
                   const llvm::function_ref<bool(OmpSsClauseKind)> CPred,
                   const llvm::function_ref<bool(OmpSsDirectiveKind)> DPred,
                   bool FromParent) const {
  D = getCanonicalDecl(D);
  iterator I = Stack.back().first.rbegin();
  iterator EndI = Stack.back().first.rend();
  if (FromParent && I != EndI)
    std::advance(I, 1);
  for (; I != EndI; std::advance(I, 1)) {
    if (!DPred(I->Directive))
      continue;
    DSAVarData DVar = getDSA(I, D);
    if (CPred(DVar.CKind))
      return DVar;
  }
  return {};
}

namespace {
class DSAAttrChecker final : public StmtVisitor<DSAAttrChecker, void> {
  DSAStackTy *Stack;
  Sema &SemaRef;
  bool ErrorFound = false;
  Stmt *CS = nullptr;
  llvm::SmallVector<Expr *, 4> ImplicitShared;
  llvm::SmallVector<Expr *, 4> ImplicitFirstprivate;

public:
  void VisitDeclRefExpr(DeclRefExpr *E) {
    if (E->isTypeDependent() || E->isValueDependent() ||
        E->containsUnexpandedParameterPack() || E->isInstantiationDependent())
      return;
    if (auto *VD = dyn_cast<VarDecl>(E->getDecl())) {
      VD = VD->getCanonicalDecl();

      DSAStackTy::DSAVarData DVarCurrent = Stack->getCurrentDSA(VD);
      DSAStackTy::DSAVarData DVarFromParent = Stack->getTopDSA(VD, /*FromParent=*/true);

      bool IsParentExplicit = DVarFromParent.RefExpr && !DVarFromParent.IsImplicit;
      bool IsCurrentExplicit = DVarCurrent.RefExpr && !DVarCurrent.IsImplicit;

      // Check if the variable has explicit DSA only set on the current
      // directive and stop analysis if it so.
      if (IsCurrentExplicit)
        return;
      // If explicit DSA comes from parent inherit it
      if (IsParentExplicit) {
          switch (DVarFromParent.CKind) {
          case OSSC_shared:
            ImplicitShared.push_back(E);
            break;
          case OSSC_private:
          case OSSC_firstprivate:
            ImplicitFirstprivate.push_back(E);
            break;
          default:
            llvm_unreachable("unexpected DSA from parent");
          }
      } else {

        OmpSsDirectiveKind DKind = Stack->getCurrentDirective();
        if (VD->hasLocalStorage()) {
          // If no default clause is present and the variable was private/local
          // in the context encountering the construct, the variable will
          // be firstprivate
          Stack->addDSA(VD, E, OSSC_firstprivate, true);

          // Define implicit data-sharing attributes for task.
          if (isOmpSsTaskingDirective(DKind))
            ImplicitFirstprivate.push_back(E);
        } else {
          // If no default clause is present and the variable was shared/global
          // in the context encountering the construct, the variable will be shared.
          Stack->addDSA(VD, E, OSSC_shared, true);

          // Define implicit data-sharing attributes for task.
          if (isOmpSsTaskingDirective(DKind))
            ImplicitShared.push_back(E);
        }
      }
    }
  }

  void VisitStmt(Stmt *S) {
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }

  bool isErrorFound() const { return ErrorFound; }

  ArrayRef<Expr *> getImplicitShared() const {
    return ImplicitShared;
  }

  ArrayRef<Expr *> getImplicitFirstprivate() const {
    return ImplicitFirstprivate;
  }

  DSAAttrChecker(DSAStackTy *S, Sema &SemaRef, Stmt *CS)
      : Stack(S), SemaRef(SemaRef), ErrorFound(false), CS(CS) {}
};
} // namespace

void Sema::InitDataSharingAttributesStackOmpSs() {
  VarDataSharingAttributesStackOmpSs = new DSAStackTy(*this);
}

#define DSAStack static_cast<DSAStackTy *>(VarDataSharingAttributesStackOmpSs)

void Sema::StartOmpSsDSABlock(OmpSsDirectiveKind DKind,
                               Scope *CurScope, SourceLocation Loc) {
  DSAStack->push(DKind, CurScope, Loc);
  PushExpressionEvaluationContext(
      ExpressionEvaluationContext::PotentiallyEvaluated);
}

void Sema::EndOmpSsDSABlock(Stmt *CurDirective) {
  DSAStack->pop();
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();
}

static std::string
getListOfPossibleValues(OmpSsClauseKind K, unsigned First, unsigned Last,
                        ArrayRef<unsigned> Exclude = llvm::None) {
  SmallString<256> Buffer;
  llvm::raw_svector_ostream Out(Buffer);
  unsigned Bound = Last >= 2 ? Last - 2 : 0;
  unsigned Skipped = Exclude.size();
  auto S = Exclude.begin(), E = Exclude.end();
  for (unsigned I = First; I < Last; ++I) {
    if (std::find(S, E, I) != E) {
      --Skipped;
      continue;
    }
    Out << "'" << getOmpSsSimpleClauseTypeName(K, I) << "'";
    if (I == Bound - Skipped)
      Out << " or ";
    else if (I != Bound + 1 - Skipped)
      Out << ", ";
  }
  return Out.str();
}

StmtResult Sema::ActOnOmpSsExecutableDirective(ArrayRef<OSSClause *> Clauses,
    OmpSsDirectiveKind Kind, Stmt *AStmt, SourceLocation StartLoc, SourceLocation EndLoc) {

  bool ErrorFound = false;
  llvm::SmallVector<OSSClause *, 8> ClausesWithImplicit;
  ClausesWithImplicit.append(Clauses.begin(), Clauses.end());
  if (AStmt) {
    // Check default data sharing attributes for referenced variables.
    DSAAttrChecker DSAChecker(DSAStack, *this, AStmt);
    Stmt *S = AStmt;
    DSAChecker.Visit(S);
    if (DSAChecker.isErrorFound())
      return StmtError();

    SmallVector<Expr *, 4> ImplicitShared(
        DSAChecker.getImplicitShared().begin(),
        DSAChecker.getImplicitShared().end());

    SmallVector<Expr *, 4> ImplicitFirstprivate(
        DSAChecker.getImplicitFirstprivate().begin(),
        DSAChecker.getImplicitFirstprivate().end());

    if (!ImplicitShared.empty()) {
      if (OSSClause *Implicit = ActOnOmpSsSharedClause(
              ImplicitShared, SourceLocation(), SourceLocation(),
              SourceLocation())) {
        ClausesWithImplicit.push_back(Implicit);
        ErrorFound = cast<OSSSharedClause>(Implicit)->varlist_size() !=
                     ImplicitShared.size();
      } else {
        ErrorFound = true;
      }
    }

    if (!ImplicitFirstprivate.empty()) {
      if (OSSClause *Implicit = ActOnOmpSsFirstprivateClause(
              ImplicitFirstprivate, SourceLocation(), SourceLocation(),
              SourceLocation())) {
        ClausesWithImplicit.push_back(Implicit);
        ErrorFound = cast<OSSFirstprivateClause>(Implicit)->varlist_size() !=
                     ImplicitFirstprivate.size();
      } else {
        ErrorFound = true;
      }
    }
  }

  StmtResult Res = StmtError();
  switch (Kind) {
  case OSSD_taskwait:
    Res = ActOnOmpSsTaskwaitDirective(StartLoc, EndLoc);
    break;
  case OSSD_task:
    Res = ActOnOmpSsTaskDirective(ClausesWithImplicit, AStmt, StartLoc, EndLoc);
    break;
  case OSSD_unknown:
    llvm_unreachable("Unknown OmpSs directive");
  }

  if (ErrorFound)
    return StmtError();

  return Res;
}

StmtResult Sema::ActOnOmpSsTaskwaitDirective(SourceLocation StartLoc,
                                             SourceLocation EndLoc) {
  return OSSTaskwaitDirective::Create(Context, StartLoc, EndLoc);
}

StmtResult Sema::ActOnOmpSsTaskDirective(ArrayRef<OSSClause *> Clauses,
                                         Stmt *AStmt,
                                         SourceLocation StartLoc,
                                         SourceLocation EndLoc) {
  if (!AStmt)
    return StmtError();
  return OSSTaskDirective::Create(Context, StartLoc, EndLoc, Clauses, AStmt);
}

OSSClause *
Sema::ActOnOmpSsDependClause(const SmallVector<OmpSsDependClauseKind, 2>& DepKinds, SourceLocation DepLoc,
                             SourceLocation ColonLoc, ArrayRef<Expr *> VarList,
                             SourceLocation StartLoc,
                             SourceLocation LParenLoc, SourceLocation EndLoc) {
  if (DepKinds.size() == 2) {
    int numWeaks = 0;
    int numUnk = 0;
    if (DepKinds[0] == OSSC_DEPEND_weak)
      ++numWeaks;
    else if (DepKinds[0] == OSSC_DEPEND_unknown)
      ++numUnk;
    if (DepKinds[1] == OSSC_DEPEND_weak)
      ++numWeaks;
    else if (DepKinds[1] == OSSC_DEPEND_unknown)
      ++numUnk;

    if (numWeaks == 0) {
      if (numUnk == 0 || numUnk == 1) {
        Diag(DepLoc, diag::err_oss_depend_weak_required);
        return nullptr;
      } else if (numUnk == 2) {
        unsigned Except[] = {OSSC_DEPEND_source, OSSC_DEPEND_sink};
        Diag(DepLoc, diag::err_oss_unexpected_clause_value)
            << getListOfPossibleValues(OSSC_depend, /*First=*/0,
                                       /*Last=*/OSSC_DEPEND_unknown, Except)
            << getOmpSsClauseName(OSSC_depend);
      }
    } else if ((numWeaks == 1 && numUnk == 1)
               || (numWeaks == 2 && numUnk == 0)) {
        unsigned Except[] = {OSSC_DEPEND_source, OSSC_DEPEND_sink, OSSC_DEPEND_weak};
        Diag(DepLoc, diag::err_oss_unexpected_clause_value)
            << getListOfPossibleValues(OSSC_depend, /*First=*/0,
                                       /*Last=*/OSSC_DEPEND_unknown, Except)
            << getOmpSsClauseName(OSSC_depend);
    }
  } else {
    if (DepKinds[0] == OSSC_DEPEND_unknown
        || DepKinds[0] == OSSC_DEPEND_weak) {
      unsigned Except[] = {OSSC_DEPEND_source, OSSC_DEPEND_weak, OSSC_DEPEND_sink};
      Diag(DepLoc, diag::err_oss_unexpected_clause_value)
          << getListOfPossibleValues(OSSC_depend, /*First=*/0,
                                     /*Last=*/OSSC_DEPEND_unknown, Except)
          << getOmpSsClauseName(OSSC_depend);
      return nullptr;
    }
  }

  for (Expr *RefExpr : VarList) {
    SourceLocation ELoc = RefExpr->getExprLoc();
    Expr *SimpleExpr = RefExpr->IgnoreParenCasts();

    auto *ASE = dyn_cast<ArraySubscriptExpr>(SimpleExpr);
    if (!RefExpr->IgnoreParenImpCasts()->isLValue() ||
        (ASE &&
         !ASE->getBase()->getType().getNonReferenceType()->isPointerType() &&
         !ASE->getBase()->getType().getNonReferenceType()->isArrayType())) {
      Diag(ELoc, diag::err_oss_expected_addressable_lvalue_or_array_item)
          << RefExpr->getSourceRange();
      continue;
    }
  }
  return OSSDependClause::Create(Context, StartLoc, LParenLoc, EndLoc,
                                 DepKinds, DepLoc, ColonLoc, VarList);
}

OSSClause *
Sema::ActOnOmpSsVarListClause(
  OmpSsClauseKind Kind, ArrayRef<Expr *> Vars,
  SourceLocation StartLoc, SourceLocation LParenLoc,
  SourceLocation ColonLoc, SourceLocation EndLoc,
  const SmallVector<OmpSsDependClauseKind, 2>& DepKinds, SourceLocation DepLinMapLoc) {

  OSSClause *Res = nullptr;
  switch (Kind) {
  case OSSC_shared:
    Res = ActOnOmpSsSharedClause(Vars, StartLoc, LParenLoc, EndLoc);
    break;
  case OSSC_private:
    Res = ActOnOmpSsPrivateClause(Vars, StartLoc, LParenLoc, EndLoc);
    break;
  case OSSC_firstprivate:
    Res = ActOnOmpSsFirstprivateClause(Vars, StartLoc, LParenLoc, EndLoc);
    break;
  case OSSC_depend:
    Res = ActOnOmpSsDependClause(DepKinds, DepLinMapLoc, ColonLoc, Vars,
                                 StartLoc, LParenLoc, EndLoc);
    break;
  default:
    llvm_unreachable("Clause is not allowed.");
  }

  return Res;
}

OSSClause *
Sema::ActOnOmpSsSharedClause(ArrayRef<Expr *> Vars,
                       SourceLocation StartLoc,
                       SourceLocation LParenLoc,
                       SourceLocation EndLoc) {
  for (Expr *RefExpr : Vars) {
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr);
    auto *ME = dyn_cast_or_null<MemberExpr>(RefExpr);
    if (DE && isa<VarDecl>(DE->getDecl())) {
      DSAStack->addDSA(DE->getDecl(), RefExpr, OSSC_shared, false);
      // OK
    }
    else if (ME && isa<FieldDecl>(ME->getMemberDecl())) {
      // KO
      llvm_unreachable("Not supported FieldDecl");
    }
    else {
      // KO
      llvm_unreachable("??");
    }

  }

  return OSSSharedClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

OSSClause *
Sema::ActOnOmpSsPrivateClause(ArrayRef<Expr *> Vars,
                       SourceLocation StartLoc,
                       SourceLocation LParenLoc,
                       SourceLocation EndLoc) {
  for (Expr *RefExpr : Vars) {
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr);
    auto *ME = dyn_cast_or_null<MemberExpr>(RefExpr);
    if (DE && isa<VarDecl>(DE->getDecl())) {
      DSAStack->addDSA(DE->getDecl(), RefExpr, OSSC_private, false);
      // OK
    }
    else if (ME && isa<FieldDecl>(ME->getMemberDecl())) {
      // KO
      llvm_unreachable("Not supported FieldDecl");
    }
    else {
      // KO
      llvm_unreachable("??");
    }

  }

  return OSSPrivateClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

OSSClause *
Sema::ActOnOmpSsFirstprivateClause(ArrayRef<Expr *> Vars,
                       SourceLocation StartLoc,
                       SourceLocation LParenLoc,
                       SourceLocation EndLoc) {
  for (Expr *RefExpr : Vars) {
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr);
    auto *ME = dyn_cast_or_null<MemberExpr>(RefExpr);
    if (DE && isa<VarDecl>(DE->getDecl())) {
      DSAStack->addDSA(getCanonicalDecl(DE->getDecl()), RefExpr, OSSC_firstprivate, false);
      // OK
    }
    else if (ME && isa<FieldDecl>(ME->getMemberDecl())) {
      // KO
      llvm_unreachable("Not supported FieldDecl");
    }
    else {
      // KO
      llvm_unreachable("??");
    }

  }

  return OSSFirstprivateClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}
