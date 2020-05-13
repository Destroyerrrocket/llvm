//===--- ExprOmpSs.h - Classes for representing expressions ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Expr interface and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_EXPROMPSS_H
#define LLVM_CLANG_AST_EXPROMPSS_H

#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"

namespace clang {
/// OmpSs-2 Array Sections.
/// To specify an array section in an OmpSs-2 construct, array subscript
/// expressions are extended with the following syntax:
/// \code
/// depend(in : [ lower-bound : length ])
/// depend(in : [ lower-bound : ])
/// depend(in : [ : length ])
/// depend(in : [ : ])
///
/// in([ lower-bound ; length ])
/// in([ lower-bound ; ])
/// in([ ; length ])
/// in([ ; ])
///
/// in([ lower-bound : upper-bound ])
/// in([ lower-bound : ])
/// in([ : upper-bound ])
/// in([ : ])
///
/// \endcode
/// The array section must be a subset of the original array.
/// Array sections are allowed on multidimensional arrays. Base language array
/// subscript expressions can be used to specify length-one dimensions of
/// multidimensional array sections.
/// The lower-bound, upper-bound and length are integral type expressions.
/// When evaluated they represent a set of integer values as follows:
/// \code
/// { lower-bound, lower-bound + 1, lower-bound + 2,... , lower-bound + length -
/// 1 }
///
/// { lower-bound, lower-bound + 1, lower-bound + 2,... , upper-bound }
/// \endcode
/// The lower-bound, upper-bound and length must evaluate to non-negative integers.
/// When the size of the array dimension is not known, the length/upper-bound
/// must be specified explicitly.
/// When the length is absent, it defaults to the size of the array dimension
/// minus the lower-bound.
/// When the upper-bound is absent, it defaults to the size of the
/// array dimension - 1
/// When the lower-bound is absent it defaults to 0.
class OSSArraySectionExpr : public Expr {
  enum { BASE, LOWER_BOUND, LENGTH_UPPER, END_EXPR };
  Stmt *SubExprs[END_EXPR];
  SourceLocation ColonLoc;
  SourceLocation RBracketLoc;
  bool ColonForm;

public:
  OSSArraySectionExpr(Expr *Base, Expr *LowerBound, Expr *LengthUpper, QualType Type,
                      ExprValueKind VK, ExprObjectKind OK,
                      SourceLocation ColonLoc, SourceLocation RBracketLoc,
                      bool ColonForm)
      : Expr(
            OSSArraySectionExprClass, Type, VK, OK),
        ColonLoc(ColonLoc), RBracketLoc(RBracketLoc), ColonForm(ColonForm) {
    SubExprs[BASE] = Base;
    SubExprs[LOWER_BOUND] = LowerBound;
    SubExprs[LENGTH_UPPER] = LengthUpper;
    setDependence(computeDependence(this));
  }

  /// Create an empty array section expression.
  explicit OSSArraySectionExpr(EmptyShell Shell)
      : Expr(OSSArraySectionExprClass, Shell) {}

  /// An array section can be written as:
  /// Base[LowerBound : Length]
  /// Base[LowerBound ; Length]
  /// Base[LowerBound : UpperBound]

  /// Get base of the array section.
  Expr *getBase() { return cast<Expr>(SubExprs[BASE]); }
  const Expr *getBase() const { return cast<Expr>(SubExprs[BASE]); }
  /// Set base of the array section.
  void setBase(Expr *E) { SubExprs[BASE] = E; }

  /// Return original type of the base expression for array section.
  static QualType getBaseOriginalType(const Expr *Base);

  /// Get lower bound of array section.
  Expr *getLowerBound() { return cast_or_null<Expr>(SubExprs[LOWER_BOUND]); }
  const Expr *getLowerBound() const {
    return cast_or_null<Expr>(SubExprs[LOWER_BOUND]);
  }
  /// Set lower bound of the array section.
  void setLowerBound(Expr *E) { SubExprs[LOWER_BOUND] = E; }

  /// Get length or upper-bound of array section.
  Expr *getLengthUpper() { return cast_or_null<Expr>(SubExprs[LENGTH_UPPER]); }
  const Expr *getLengthUpper() const { return cast_or_null<Expr>(SubExprs[LENGTH_UPPER]); }
  /// Set length or upper-bound of the array section.
  void setLengthUpper(Expr *E) { SubExprs[LENGTH_UPPER] = E; }

  // Get section form ';' or ':'
  bool isColonForm() const { return ColonForm; }

  SourceLocation getBeginLoc() const LLVM_READONLY {
    return getBase()->getBeginLoc();
  }
  SourceLocation getEndLoc() const LLVM_READONLY { return RBracketLoc; }

  SourceLocation getColonLoc() const { return ColonLoc; }
  void setColonLoc(SourceLocation L) { ColonLoc = L; }

  SourceLocation getRBracketLoc() const { return RBracketLoc; }
  void setRBracketLoc(SourceLocation L) { RBracketLoc = L; }

  SourceLocation getExprLoc() const LLVM_READONLY {
    return getBase()->getExprLoc();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == OSSArraySectionExprClass;
  }

  child_range children() {
    return child_range(&SubExprs[BASE], &SubExprs[END_EXPR]);
  }

  const_child_range children() const {
    return const_child_range(&SubExprs[BASE], &SubExprs[END_EXPR]);
  }
};

// TODO: documentation
class OSSArrayShapingExpr final
  : public Expr,
    private llvm::TrailingObjects<OSSArrayShapingExpr, Expr *> {

  friend TrailingObjects;

  unsigned NumShapes;

  SourceLocation BeginLoc;
  SourceLocation EndLoc;

  size_t numTrailingObjects(OverloadToken<Stmt *>) const {
    // Add an extra one for the base expression.
    return NumShapes + 1;
  }

  OSSArrayShapingExpr(QualType Type, Expr *Base, ArrayRef<Expr *> ShapeList,
                      ExprValueKind VK, ExprObjectKind OK, unsigned N,
                      SourceLocation BeginLoc, SourceLocation EndLoc)
      : Expr( OSSArrayShapingExprClass, Type, VK, OK),
            NumShapes(N), BeginLoc(BeginLoc), EndLoc(EndLoc) {
    setBase(Base);
    setShapes(ShapeList);
    setDependence(computeDependence(this));
  }

  /// Create an empty array section expression.
  explicit OSSArrayShapingExpr(EmptyShell Shell, unsigned N)
      : Expr(OSSArrayShapingExprClass, Shell), NumShapes(N) {}

public:

  static OSSArrayShapingExpr *Create(const ASTContext &C,
                                 QualType Type,
                                 ExprValueKind VK,
                                 ExprObjectKind OK,
                                 Expr *Base,
                                 ArrayRef<Expr *> ShapeList,
                                 SourceLocation BeginLoc,
                                 SourceLocation EndLoc) {
    void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(ShapeList.size() + 1));
    OSSArrayShapingExpr *Clause = new (Mem)
        OSSArrayShapingExpr(Type, Base, ShapeList, VK, OK, ShapeList.size(), BeginLoc, EndLoc);
    return Clause;
  }

  /// Get base of the array section.
  Expr *getBase() { return getTrailingObjects<Expr *>()[0]; }
  const Expr *getBase() const { return getTrailingObjects<Expr *>()[0]; }
  /// Set base of the array section.
  void setBase(Expr *E) { getTrailingObjects<Expr *>()[0] = E; }

  /// Get the shape of array shaping.
  MutableArrayRef<Expr *> getShapes() {
    return MutableArrayRef<Expr *>(
        getTrailingObjects<Expr *>() + 1, NumShapes);
  }
  ArrayRef<const Expr *> getShapes() const {
    return ArrayRef<Expr *>(
        getTrailingObjects<Expr *>() + 1, NumShapes);
  }
  /// Set the shape of the array shaping.
  void setShapes(ArrayRef<Expr *> VL) {
    std::copy(VL.begin(), VL.end(),
              getTrailingObjects<Expr *>() + 1);
  }

  SourceLocation getBeginLoc() const LLVM_READONLY { return BeginLoc; }
  SourceLocation getEndLoc() const LLVM_READONLY { return EndLoc; }
  SourceRange getSourceRange() const LLVM_READONLY { return SourceRange(BeginLoc, EndLoc); }

  SourceLocation getExprLoc() const LLVM_READONLY {
    return getBase()->getBeginLoc();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == OSSArrayShapingExprClass;
  }

  child_range children() {
    Stmt **Begin = reinterpret_cast<Stmt **>(getTrailingObjects<Expr *>());
    return child_range(Begin, Begin + NumShapes + 1);
  }

  const_child_range children() const {
    Stmt *const *Begin =
        reinterpret_cast<Stmt *const *>(getTrailingObjects<Expr *>());
    return const_child_range(Begin, Begin + NumShapes + 1);
  }
};
} // end namespace clang

#endif
