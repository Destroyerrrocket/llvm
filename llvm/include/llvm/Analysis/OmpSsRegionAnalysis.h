//===- llvm/Analysis/OmpSsRegionAnalysis.h - OmpSs Region Analysis -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_ANALYSIS_OMPSSREGIONANALYSIS_H
#define LLVM_ANALYSIS_OMPSSREGIONANALYSIS_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Dominators.h"

namespace llvm {

// Task data structures
struct TaskDSAInfo {
  SetVector<Value *> Shared;
  SetVector<Value *> Private;
  SetVector<Value *> Firstprivate;
};

// <VLA, VLA_dims>
using TaskVLADimsInfo = MapVector<Value *, SetVector<Value *>>;
using TaskCapturedInfo = SetVector<Value *>;


// Non-POD stuff
using TaskInits = MapVector<Value *, Value *>;
using TaskDeinits = MapVector<Value *, Value *>;
using TaskCopies = MapVector<Value *, Value *>;

struct TaskNonPODsInfo {
  TaskInits Inits;
  TaskDeinits Deinits;
  TaskCopies Copies;
};

struct DependInfo {
  int SymbolIndex;
  std::string RegionText;
  Value *Base;
  SmallVector<Value *, 4> Dims;
};

struct TaskDependsInfo {
  SmallVector<DependInfo, 4> Ins;
  SmallVector<DependInfo, 4> Outs;
  SmallVector<DependInfo, 4> Inouts;
  SmallVector<DependInfo, 4> Concurrents;
  SmallVector<DependInfo, 4> Commutatives;
  SmallVector<DependInfo, 4> WeakIns;
  SmallVector<DependInfo, 4> WeakOuts;
  SmallVector<DependInfo, 4> WeakInouts;

  // Unique Instructions needed as a bridge between dependency and DSAs
  // Stored in program order
  SmallVector<Instruction *, 4> UnpackInstructions;
  // Unique ConstantExpr needed between dependency and DSAs
  SetVector<ConstantExpr *> UnpackConstants;
  int NumSymbols;
};

struct TaskInfo {
  TaskDSAInfo DSAInfo;
  TaskVLADimsInfo VLADimsInfo;
  TaskDependsInfo DependsInfo;
  Value *Final = nullptr;
  Value *If = nullptr;
  TaskCapturedInfo CapturedInfo;
  TaskNonPODsInfo NonPODsInfo;
  Instruction *Entry;
  Instruction *Exit;
};

struct TaskFunctionInfo {
  SmallVector<TaskInfo, 4> PostOrder;
};
// End Task data structures

// Taskwait data structures
struct TaskwaitInfo {
  Instruction *I;
};

struct TaskwaitFunctionInfo {
  SmallVector<TaskwaitInfo, 4> PostOrder;
};
// End Taskwait data structures

struct FunctionInfo {
  TaskFunctionInfo TaskFuncInfo;
  TaskwaitFunctionInfo TaskwaitFuncInfo;
};

// Start Analysis data structures. this info is not passed to transformation phase
struct TaskPrintInfo {
  // Task nesting level
  size_t Depth;
  size_t Idx;
};

struct TaskAnalysisInfo {
  SetVector<Value *> UsesBeforeEntry;
  SetVector<Value *> UsesAfterExit;
  // Map of Dependency symbols to Index
  std::map<Value *, int> DepSymToIdx;
};

struct TaskFunctionAnalysisInfo {
  SmallVector<TaskAnalysisInfo, 4> PostOrder;
};

// End Analysis data structures

class OmpSsRegionAnalysisPass : public FunctionPass {
private:


  // AnalysisInfo only used in this pass
  TaskFunctionAnalysisInfo TaskFuncAnalysisInfo;
  // Info used by the transform pass
  FunctionInfo FuncInfo;
  // Used to keep the task layout to print it properly, since
  // we save the tasks in post order
  MapVector<Instruction *, TaskPrintInfo> TaskProgramOrder;

  static const int PrintSpaceMultiplier = 2;
  // Walk over each task in RPO identifying uses before entry
  // and after exit. Uses before task entry are then matched with DSA info
  // in OperandBundles
  // Also, gathers all taskwait instructions
  static void getOmpSsFunctionInfo(
      Function &F, DominatorTree &DT, FunctionInfo &FI,
      TaskFunctionAnalysisInfo &TFAI,
      MapVector<Instruction *, TaskPrintInfo> &TPO);

public:
  static char ID;

  OmpSsRegionAnalysisPass();

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "OmpSs-2 Region Analysis"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  void print(raw_ostream &OS, const Module *M) const override;

  void releaseMemory() override;

  FunctionInfo& getFuncInfo();

};

} // end namespace llvm

#endif // LLVM_ANALYSIS_OMPSSREGIONANALYSIS_H

