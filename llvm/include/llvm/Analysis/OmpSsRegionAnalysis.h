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
  // Map of Dependency symbols to Index
  std::map<Value *, int> DepSymToIdx;
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
  Value *Base;
  Function *ComputeDepFun;
  SmallVector<Value *, 4> Args;
  int SymbolIndex;
  std::string RegionText;
};

struct MultiDependInfo {
  SmallVector<Value *, 4> Iters;
  Function *ComputeMultiDepFun;
  SmallVector<Value *, 4> Args;
  DependInfo DepInfo;
};

struct ReductionInfo {
  Value *RedKind;
  DependInfo DepInfo;
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
  SmallVector<DependInfo, 4> WeakConcurrents;
  SmallVector<DependInfo, 4> WeakCommutatives;
  SmallVector<ReductionInfo, 4> Reductions;
  SmallVector<ReductionInfo, 4> WeakReductions;

  SmallVector<MultiDependInfo, 4> MultiRangeIns;
  SmallVector<MultiDependInfo, 4> MultiRangeOuts;
  SmallVector<MultiDependInfo, 4> MultiRangeInouts;
  SmallVector<MultiDependInfo, 4> MultiRangeConcurrents;
  SmallVector<MultiDependInfo, 4> MultiRangeCommutatives;
  SmallVector<MultiDependInfo, 4> MultiRangeWeakIns;
  SmallVector<MultiDependInfo, 4> MultiRangeWeakOuts;
  SmallVector<MultiDependInfo, 4> MultiRangeWeakInouts;
  SmallVector<MultiDependInfo, 4> MultiRangeWeakConcurrents;
  SmallVector<MultiDependInfo, 4> MultiRangeWeakCommutatives;

  int NumSymbols;
};

struct ReductionInitCombInfo {
  Value *Init;
  Value *Comb;
  // This is used to index the array of
  // init/combiners
  int ReductionIndex;
};

using TaskReductionsInitCombInfo = MapVector<Value *, ReductionInitCombInfo>;

struct TaskLoopInfo {
  // TODO document this
  enum { LT, LE, GT, GE };
  int64_t LoopType;
  int64_t IndVarSigned;
  int64_t LBoundSigned;
  int64_t UBoundSigned;
  int64_t StepSigned;
  Value *IndVar = nullptr;
  Value *LBound = nullptr;
  Value *UBound = nullptr;
  Value *Step = nullptr;
  Value *Chunksize = nullptr;
  Value *Grainsize = nullptr;
  bool empty() const {
    return !IndVar && !LBound &&
           !UBound && !Step;
  }
};

struct TaskInfo {
  enum OmpSsTaskKind {
    OSSD_task = 0,
    OSSD_task_for,
    OSSD_taskloop,
    OSSD_taskloop_for,
    OSSD_unknown
  };
  OmpSsTaskKind TaskKind = OSSD_unknown;
  TaskDSAInfo DSAInfo;
  TaskVLADimsInfo VLADimsInfo;
  TaskDependsInfo DependsInfo;
  TaskReductionsInitCombInfo ReductionsInitCombInfo;
  Value *Final = nullptr;
  Value *If = nullptr;
  Value *Priority = nullptr;
  Value *Label = nullptr;
  Value *Cost = nullptr;
  Value *Wait = nullptr;
  TaskCapturedInfo CapturedInfo;
  TaskNonPODsInfo NonPODsInfo;
  // This is not taskloop only info
  TaskLoopInfo LoopInfo;
  // Used to lower directives in final context.
  // Used to build loops of taskloop/taskfor
  SmallVector<TaskInfo *, 4> InnerTaskInfos;
  Instruction *Entry;
  Instruction *Exit;
};

struct TaskFunctionInfo {
  SmallVector<TaskInfo *, 4> PostOrder;
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

// Release data structures
struct ReleaseDependInfo {
  Value *Base;
  Function *ComputeDepFun;
  SmallVector<Value *, 4> Args;
};

struct ReleaseDependsInfo {
  SmallVector<ReleaseDependInfo, 4> Ins;
  SmallVector<ReleaseDependInfo, 4> Outs;
  SmallVector<ReleaseDependInfo, 4> Inouts;
  SmallVector<ReleaseDependInfo, 4> WeakIns;
  SmallVector<ReleaseDependInfo, 4> WeakOuts;
  SmallVector<ReleaseDependInfo, 4> WeakInouts;
};

struct ReleaseInfo {
  ReleaseDependsInfo DependsInfo;
  Instruction *I;
};

struct ReleaseFunctionInfo {
  SmallVector<ReleaseInfo, 4> PostOrder;
};
// End Release data structures

// Start Analysis data structures. this info is not passed to transformation phase
struct TaskAnalysisInfo {
  SetVector<Value *> UsesBeforeEntry;
  SetVector<Value *> UsesAfterExit;
};

struct TaskWithAnalysisInfo {
  TaskAnalysisInfo AnalysisInfo;
  TaskInfo Info;
};

// End Analysis data structures

struct FunctionInfo {
  TaskFunctionInfo TaskFuncInfo;
  TaskwaitFunctionInfo TaskwaitFuncInfo;
  ReleaseFunctionInfo ReleaseFuncInfo;
};

class OmpSsRegionAnalysisPass : public FunctionPass {
private:

  // Task Analysis and Info for a task entry
  MapVector<Instruction *, TaskWithAnalysisInfo> TEntryToTaskWithAnalysisInfo;
  // nullptr is the first level where the outer tasks are
  MapVector<Instruction *, SmallVector<Instruction *, 4>> TasksTree;

  // Info used by the transform pass
  FunctionInfo FuncInfo;

  static const int PrintSpaceMultiplier = 2;
  // Walk over each task in RPO identifying uses before entry
  // and after exit. Uses before task entry are then matched with DSA info
  // in OperandBundles
  // Also, gathers all taskwait instructions
  static void getOmpSsFunctionInfo(
      Function &F, DominatorTree &DT, FunctionInfo &FI,
      MapVector<Instruction *, TaskWithAnalysisInfo> &TEntryToTaskWithAnalysisInfo,
      MapVector<Instruction *, SmallVector<Instruction *, 4>> &TasksTree);

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

