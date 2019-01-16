//===- StmtOmpSs.h - Classes for OmpSs directives  ------------*- C++ -*-===//
//
//                     The LLVM Cossiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines OmpSs AST classes for executable directives and
/// clauses.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_STMTOMPSS_H
#define LLVM_CLANG_AST_STMTOMPSS_H

#include "clang/AST/Expr.h"
#include "clang/AST/OmpSsClause.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/OmpSsKinds.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

//===----------------------------------------------------------------------===//
// AST classes for directives.
//===----------------------------------------------------------------------===//

/// This is a basic class for representing single OmpSs executable
/// directive.
///
class OSSExecutableDirective : public Stmt {
  friend class ASTStmtReader;
  /// Kind of the directive.
  OmpSsDirectiveKind Kind;
  /// Starting location of the directive (directive keyword).
  SourceLocation StartLoc;
  /// Ending location of the directive.
  SourceLocation EndLoc;
  /// Numbers of clauses.
  const unsigned NumClauses;
  /// Number of child expressions/stmts.
  const unsigned NumChildren;
  /// Offset from this to the start of clauses.
  /// There are NumClauses pointers to clauses, they are followed by
  /// NumChildren pointers to child stmts/exprs (if the directive type
  /// requires an associated stmt, then it has to be the first of them).
  const unsigned ClausesOffset;

  /// Get the clauses storage.
  MutableArrayRef<OSSClause *> getClauses() {
    OSSClause **ClauseStorage = reinterpret_cast<OSSClause **>(
        reinterpret_cast<char *>(this) + ClausesOffset);
    return MutableArrayRef<OSSClause *>(ClauseStorage, NumClauses);
  }
protected:
  /// Build instance of directive of class \a K.
  ///
  /// \param SC Statement class.
  /// \param K Kind of OmpSs directive.
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending location of the directive.
  ///
  template <typename T>
  OSSExecutableDirective(const T *, StmtClass SC, OmpSsDirectiveKind K,
                         SourceLocation StartLoc, SourceLocation EndLoc,
                         unsigned NumClauses, unsigned NumChildren)
      : Stmt(SC), Kind(K), StartLoc(std::move(StartLoc)),
        EndLoc(std::move(EndLoc)), NumClauses(NumClauses),
        NumChildren(NumChildren),
        ClausesOffset(llvm::alignTo(sizeof(T), alignof(OSSClause *))) {}

  /// Sets the list of variables for this clause.
  ///
  /// \param Clauses The list of clauses for the directive.
  ///
  void setClauses(ArrayRef<OSSClause *> Clauses);

  /// Set the associated statement for the directive.
  ///
  /// /param S Associated statement.
  ///
  void setAssociatedStmt(Stmt *S) {
    assert(hasAssociatedStmt() && "no associated statement.");
    *child_begin() = S;
  }

public:
  /// Iterates over a filtered subrange of clauses applied to a
  /// directive.
  ///
  /// This iterator visits only clauses of type SpecificClause.
  template <typename SpecificClause>
  class specific_clause_iterator
      : public llvm::iterator_adaptor_base<
            specific_clause_iterator<SpecificClause>,
            ArrayRef<OSSClause *>::const_iterator, std::forward_iterator_tag,
            const SpecificClause *, ptrdiff_t, const SpecificClause *,
            const SpecificClause *> {
    ArrayRef<OSSClause *>::const_iterator End;

    void SkipToNextClause() {
      while (this->I != End && !isa<SpecificClause>(*this->I))
        ++this->I;
    }

  public:
    explicit specific_clause_iterator(ArrayRef<OSSClause *> Clauses)
        : specific_clause_iterator::iterator_adaptor_base(Clauses.begin()),
          End(Clauses.end()) {
      SkipToNextClause();
    }

    const SpecificClause *operator*() const {
      return cast<SpecificClause>(*this->I);
    }
    const SpecificClause *operator->() const { return **this; }

    specific_clause_iterator &operator++() {
      ++this->I;
      SkipToNextClause();
      return *this;
    }
  };

  template <typename SpecificClause>
  static llvm::iterator_range<specific_clause_iterator<SpecificClause>>
  getClausesOfKind(ArrayRef<OSSClause *> Clauses) {
    return {specific_clause_iterator<SpecificClause>(Clauses),
            specific_clause_iterator<SpecificClause>(
                llvm::makeArrayRef(Clauses.end(), 0))};
  }

  template <typename SpecificClause>
  llvm::iterator_range<specific_clause_iterator<SpecificClause>>
  getClausesOfKind() const {
    return getClausesOfKind<SpecificClause>(clauses());
  }

  /// Gets a single clause of the specified kind associated with the
  /// current directive iff there is only one clause of this kind (and assertion
  /// is fired if there is more than one clause is associated with the
  /// directive). Returns nullptr if no clause of this kind is associated with
  /// the directive.
  template <typename SpecificClause>
  const SpecificClause *getSingleClause() const {
    auto Clauses = getClausesOfKind<SpecificClause>();

    if (Clauses.begin() != Clauses.end()) {
      assert(std::next(Clauses.begin()) == Clauses.end() &&
             "There are at least 2 clauses of the specified kind");
      return *Clauses.begin();
    }
    return nullptr;
  }

  /// Returns true if the current directive has one or more clauses of a
  /// specific kind.
  template <typename SpecificClause>
  bool hasClausesOfKind() const {
    auto Clauses = getClausesOfKind<SpecificClause>();
    return Clauses.begin() != Clauses.end();
  }

  /// Returns starting location of directive kind.
  SourceLocation getBeginLoc() const { return StartLoc; }
  /// Returns ending location of directive.
  SourceLocation getEndLoc() const { return EndLoc; }

  /// Set starting location of directive kind.
  ///
  /// \param Loc New starting location of directive.
  ///
  void setLocStart(SourceLocation Loc) { StartLoc = Loc; }
  /// Set ending location of directive.
  ///
  /// \param Loc New ending location of directive.
  ///
  void setLocEnd(SourceLocation Loc) { EndLoc = Loc; }

  /// Get number of clauses.
  unsigned getNumClauses() const { return NumClauses; }

  /// Returns specified clause.
  ///
  /// \param i Number of clause.
  ///
  OSSClause *getClause(unsigned i) const { return clauses()[i]; }

  /// Returns true if directive has associated statement.
  bool hasAssociatedStmt() const { return NumChildren > 0; }

  /// Returns statement associated with the directive.
  const Stmt *getAssociatedStmt() const {
    assert(hasAssociatedStmt() && "no associated statement.");
    return *child_begin();
  }
  Stmt *getAssociatedStmt() {
    assert(hasAssociatedStmt() && "no associated statement.");
    return *child_begin();
  }

  OmpSsDirectiveKind getDirectiveKind() const { return Kind; }

  static bool classof(const Stmt *S) {
    return S->getStmtClass() >= firstOSSExecutableDirectiveConstant &&
           S->getStmtClass() <= lastOSSExecutableDirectiveConstant;
  }

  child_range children() {
    if (!hasAssociatedStmt())
      return child_range(child_iterator(), child_iterator());
    Stmt **ChildStorage = reinterpret_cast<Stmt **>(getClauses().end());
    /// Do not mark all the special expression/statements as children, except
    /// for the associated statement.
    return child_range(ChildStorage, ChildStorage + 1);
  }

  ArrayRef<OSSClause *> clauses() { return getClauses(); }

  ArrayRef<OSSClause *> clauses() const {
    return const_cast<OSSExecutableDirective *>(this)->getClauses();
  }
};

/// This represents '#pragma oss taskwait' directive.
///
/// \code
/// #pragma oss taskwait
/// \endcode
///
class OSSTaskwaitDirective : public OSSExecutableDirective {
  friend class ASTStmtReader;
  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending location of the directive.
  ///
  OSSTaskwaitDirective(SourceLocation StartLoc, SourceLocation EndLoc)
      : OSSExecutableDirective(this, OSSTaskwaitDirectiveClass, OSSD_taskwait,
                               StartLoc, EndLoc, 0, 0) {}

  /// Build an empty directive.
  ///
  explicit OSSTaskwaitDirective()
      : OSSExecutableDirective(this, OSSTaskwaitDirectiveClass, OSSD_taskwait,
                               SourceLocation(), SourceLocation(), 0, 0) {}

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  ///
  static OSSTaskwaitDirective *
  Create(const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  ///
  static OSSTaskwaitDirective *CreateEmpty(const ASTContext &C, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == OSSTaskwaitDirectiveClass;
  }
};

class OSSTaskDirective : public OSSExecutableDirective {
  friend class ASTStmtReader;
  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending location of the directive.
  /// \param NumClauses Number of clauses.
  ///
  OSSTaskDirective(SourceLocation StartLoc, SourceLocation EndLoc, unsigned NumClauses)
      : OSSExecutableDirective(this, OSSTaskDirectiveClass, OSSD_task,
                               StartLoc, EndLoc, NumClauses, 1) {}

  /// Build an empty directive.
  ///
  /// \param NumClauses Number of clauses.
  ///
  explicit OSSTaskDirective(unsigned NumClauses)
      : OSSExecutableDirective(this, OSSTaskDirectiveClass, OSSD_task,
                               SourceLocation(), SourceLocation(), NumClauses, 1) {}

public:
  /// Creates directive with a list of \a Clauses.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  ///
  static OSSTaskDirective *Create(const ASTContext &C, SourceLocation StartLoc,
                                  SourceLocation EndLoc,
                                  ArrayRef<OSSClause *> Clauses,
                                  Stmt *AStmt);

  /// Creates an empty directive with the place for \a NumClauses
  /// clauses.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static OSSTaskDirective *CreateEmpty(const ASTContext &C, unsigned NumClauses,
                                       EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == OSSTaskDirectiveClass;
  }
};

} // end namespace clang

#endif
