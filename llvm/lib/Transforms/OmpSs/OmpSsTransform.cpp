//===- OmpSs.cpp -- Strip parts of Debug Info --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/OmpSs.h"
#include "llvm/Analysis/OmpSsRegionAnalysis.h"

#include "llvm/Pass.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
using namespace llvm;

namespace {

struct OmpSs : public ModulePass {
  /// Pass identification, replacement for typeid
  static char ID;
  OmpSs() : ModulePass(ID) {
    initializeOmpSsPass(*PassRegistry::getPassRegistry());
  }

  bool Initialized = false;

  class Nanos6TaskAddrTranslationEntry {
  private:
    StructType *Ty;

    Nanos6TaskAddrTranslationEntry(){};
    Nanos6TaskAddrTranslationEntry(const Nanos6TaskAddrTranslationEntry&){};
  public:
    ~Nanos6TaskAddrTranslationEntry(){};

    static auto getInstance(Module &M) -> Nanos6TaskAddrTranslationEntry& {
      static auto instance = std::unique_ptr<Nanos6TaskAddrTranslationEntry>(nullptr);
      if (!instance) {
        instance.reset(new Nanos6TaskAddrTranslationEntry);
        instance->Ty = StructType::create(M.getContext(),
          "nanos6_address_translation_entry_t");
      }
      return *instance.get();
    }
    StructType *getType() { return Ty; }
  };

  class Nanos6TaskConstraints {
  private:
    StructType *Ty;

    Nanos6TaskConstraints(){};
    Nanos6TaskConstraints(const Nanos6TaskConstraints&){};
  public:
    ~Nanos6TaskConstraints(){};

    static auto getInstance(Module &M) -> Nanos6TaskConstraints& {
      static auto instance = std::unique_ptr<Nanos6TaskConstraints>(nullptr);
      if (!instance) {
        instance.reset(new Nanos6TaskConstraints);
        instance->Ty = StructType::create(M.getContext(),
          "nanos6_task_constraints_t");

        // size_t cost
        Type *CostTy = Type::getInt64Ty(M.getContext());

        instance->Ty->setBody(CostTy);
      }
      return *instance.get();
    }
    StructType *getType() { return Ty; }
  };

  class Nanos6TaskInvInfo {
  private:
    StructType *Ty;

    Nanos6TaskInvInfo(){};
    Nanos6TaskInvInfo(const Nanos6TaskInvInfo&){};
  public:
    ~Nanos6TaskInvInfo(){};

    static auto getInstance(Module &M) -> Nanos6TaskInvInfo& {
      static auto instance = std::unique_ptr<Nanos6TaskInvInfo>(nullptr);
      if (!instance) {
        instance.reset(new Nanos6TaskInvInfo);
        instance->Ty = StructType::create(M.getContext(),
          "nanos6_task_invocation_info_t");

        // const char *invocation_source
        Type *InvSourceTy = Type::getInt8PtrTy(M.getContext());

        instance->Ty->setBody(InvSourceTy);
      }
      return *instance.get();
    }
    StructType *getType() { return Ty; }
  };

  class Nanos6TaskImplInfo {
  private:
    StructType *Ty;

    Nanos6TaskImplInfo(){};
    Nanos6TaskImplInfo(const Nanos6TaskImplInfo&){};
  public:
    ~Nanos6TaskImplInfo(){};

    static auto getInstance(Module &M) -> Nanos6TaskImplInfo& {
      static auto instance = std::unique_ptr<Nanos6TaskImplInfo>(nullptr);
      if (!instance) {
        instance.reset(new Nanos6TaskImplInfo);
        instance->Ty = StructType::create(M.getContext(),
          "nanos6_task_implementation_info_t");

        // int device_type_id;
        Type *DeviceTypeIdTy = Type::getInt32Ty(M.getContext());
        // void (*run)(void *, void *, nanos6_address_translation_entry_t *);
        Type *RunFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        // void (*get_constraints)(void *, nanos6_task_constraints_t *);
        Type *GetConstraintsFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        // const char *task_label;
        Type *TaskLabelTy = Type::getInt8PtrTy(M.getContext());
        // const char *declaration_source;
        Type *DeclSourceTy = Type::getInt8PtrTy(M.getContext());
        // void (*run_wrapper)(void *, void *, nanos6_address_translation_entry_t *);
        Type *RunWrapperFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        instance->Ty->setBody({DeviceTypeIdTy, RunFuncTy,
                              GetConstraintsFuncTy, TaskLabelTy,
                              DeclSourceTy, RunWrapperFuncTy});
      }
      return *instance.get();
    }
    StructType *getType() { return Ty; }
  };

  class Nanos6TaskInfo {
  private:
    StructType *Ty;

    Nanos6TaskInfo(){};
    Nanos6TaskInfo(const Nanos6TaskInfo&){};
  public:
    ~Nanos6TaskInfo(){};

    static auto getInstance(Module &M) -> Nanos6TaskInfo& {
      static auto instance = std::unique_ptr<Nanos6TaskInfo>(nullptr);
      if (!instance) {
        instance.reset(new Nanos6TaskInfo);
        instance->Ty = StructType::create(M.getContext(),
          "nanos6_task_info_t");

        // int num_symbols;
        Type *NumSymbolsTy = Type::getInt32Ty(M.getContext());;
        // void (*register_depinfo)(void *, void *);
        Type *RegisterInfoFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        // void (*get_priority)(void *, nanos6_priority_t *);
        // void (*get_priority)(void *, long int *);
        Type *GetPriorityFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        // const char *type_identifier;
        Type *TypeIdTy = Type::getInt8PtrTy(M.getContext());
        // int implementation_count;
        Type *ImplCountTy = Type::getInt32Ty(M.getContext());
        // nanos6_task_implementation_info_t *implementations;
        Type *TaskImplInfoTy = StructType::get(M.getContext())->getPointerTo();
        // void (*destroy_args_block)(void *);
        Type *DestroyArgsBlockFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        // void (*duplicate_args_block)(const void *, void **);
        Type *DuplicateArgsBlockFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo();
        // void (**reduction_initializers)(void *, void *, size_t);
        Type *ReductInitsFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo()->getPointerTo();
        // void (**reduction_combiners)(void *, void *, size_t);
        Type *ReductCombsFuncTy =
          FunctionType::get(Type::getVoidTy(M.getContext()),
                            /*IsVarArgs=*/false)->getPointerTo()->getPointerTo();

        instance->Ty->setBody({NumSymbolsTy, RegisterInfoFuncTy, GetPriorityFuncTy, TypeIdTy,
                               ImplCountTy, TaskImplInfoTy, DestroyArgsBlockFuncTy,
                               DuplicateArgsBlockFuncTy, ReductInitsFuncTy, ReductCombsFuncTy,
                              });
      }
      return *instance.get();
    }
    StructType *getType() { return Ty; }
  };

  class Nanos6MultidepFactory {
    const size_t MAX_DEP_DIMS = 8;
  private:
    StringMap<FunctionCallee> DepNameToFuncCalleeMap;

    FunctionType *BuildDepFuncType(Module &M, StringRef FullName, size_t Ndims) {
      // void nanos6_register_region_X_depinfoY(
      //   void *handler, int symbol_index, char const *region_text,
      //   void *base_address,
      //   long dim1size, long dim1start, long dim1end,
      //   ...);
      SmallVector<Type *, 8> Params = {
        Type::getInt8PtrTy(M.getContext()),
        Type::getInt32Ty(M.getContext()),
        Type::getInt8PtrTy(M.getContext()),
        Type::getInt8PtrTy(M.getContext())
      };
      for (size_t i = 0; i < Ndims; ++i) {
        // long dimsize
        Params.push_back(Type::getInt64Ty(M.getContext()));
        // long dimstart
        Params.push_back(Type::getInt64Ty(M.getContext()));
        // long dimend
        Params.push_back(Type::getInt64Ty(M.getContext()));
      }
      return FunctionType::get(Type::getVoidTy(M.getContext()),
                               Params, /*IsVarArgs=*/false);
    }
  public:
    FunctionCallee getMultidepFuncCallee(Module &M, StringRef &Name, size_t Ndims) {
      std::string FullName = ("nanos6_register_region_" + Name + "_depinfo" + Twine(Ndims)).str();

      auto It = DepNameToFuncCalleeMap.find(FullName);
      if (It != DepNameToFuncCalleeMap.end())
        return It->second;

      assert(Ndims <= MAX_DEP_DIMS);

      FunctionType *DepF = BuildDepFuncType(M, FullName, Ndims);
      FunctionCallee DepCallee = M.getOrInsertFunction(FullName, DepF);
      DepNameToFuncCalleeMap[FullName] = DepCallee;
      return DepCallee;
    }
  };
  Nanos6MultidepFactory MultidepFactory;

  FunctionCallee CreateTaskFuncTy;
  FunctionCallee TaskSubmitFuncTy;
  FunctionCallee TaskInFinalFuncTy;

  void unpackDepsAndRewrite(Module &M, TaskDependsInfo &TDI, Function *F, ArrayRef<Value *> TaskArgsList) {
    BasicBlock::Create(M.getContext(), "entry", F);
    BasicBlock &Entry = F->getEntryBlock();
    DenseMap<Value *, Value *> ConstExprToInst;
    DenseMap<Value *, Value *> InstToClone;
    for (ConstantExpr * const &CE : TDI.UnpackConstants) {
      Instruction *I = CE->getAsInstruction();
      Entry.getInstList().push_back(I);

      // TODO: make a use case that have two or more same constants
      ConstExprToInst[CE] = I;
    }
    for (Instruction * const &I : TDI.UnpackInstructions) {
      Instruction *IClone = I->clone();
      if (I->hasName())
        IClone->setName(I->getName());
      Entry.getInstList().push_back(IClone);
      InstToClone[I] = IClone;
    }
    // Once we have inserted the cloned instructions and the ConstantExpr instructions
    // add the terminator so IRBuilder inserts just before it
    F->getEntryBlock().getInstList().push_back(ReturnInst::Create(M.getContext()));

    // Insert RT call before replacing uses
    unpackDepsCallToRT(M, ConstExprToInst, TDI, F);

    for (Instruction &I : Entry) {
      Function::arg_iterator AI = F->arg_begin();
      for (unsigned i = 0, e = TaskArgsList.size(); i != e; ++i, ++AI) {
        I.replaceUsesOfWith(TaskArgsList[i], &*AI);
      }
      for (auto &p : ConstExprToInst) {
        I.replaceUsesOfWith(p.first, p.second);
      }
      for (auto &p : InstToClone) {
        I.replaceUsesOfWith(p.first, p.second);
      }
    }
  }

  void unpackCostAndRewrite(Module &M, Value *Cost, Function *F, ArrayRef<Value *> TaskArgsList) {
    BasicBlock::Create(M.getContext(), "entry", F);
    BasicBlock &Entry = F->getEntryBlock();
    F->getEntryBlock().getInstList().push_back(ReturnInst::Create(M.getContext()));
    IRBuilder<> BBBuilder(&F->getEntryBlock().back());
    Value *Constraints = &*(F->arg_end() - 1);
    Value *Idx[2];
    Idx[0] = Constant::getNullValue(Type::getInt32Ty(M.getContext()));
    Idx[1] = Constant::getNullValue(Type::getInt32Ty(M.getContext()));

    Value *GEPConstraints = BBBuilder.CreateGEP(
          Constraints, Idx, "gep_" + Constraints->getName());
    Value *CostCast = BBBuilder.CreateZExt(Cost, Nanos6TaskConstraints::getInstance(M).getType()->getElementType(0));
    BBBuilder.CreateStore(CostCast, GEPConstraints);
    for (Instruction &I : Entry) {
      Function::arg_iterator AI = F->arg_begin();
      for (unsigned i = 0, e = TaskArgsList.size(); i != e; ++i, ++AI) {
        I.replaceUsesOfWith(TaskArgsList[i], &*AI);
      }
    }
  }

  void unpackPriorityAndRewrite(Module &M, Value *Priority, Function *F, ArrayRef<Value *> TaskArgsList) {
    BasicBlock::Create(M.getContext(), "entry", F);
    BasicBlock &Entry = F->getEntryBlock();
    F->getEntryBlock().getInstList().push_back(ReturnInst::Create(M.getContext()));
    IRBuilder<> BBBuilder(&F->getEntryBlock().back());
    Value *PriorityArg = &*(F->arg_end() - 1);
    Value *PrioritySExt = BBBuilder.CreateSExt(Priority, Type::getInt64Ty(M.getContext()));
    BBBuilder.CreateStore(PrioritySExt, PriorityArg);
    for (Instruction &I : Entry) {
      Function::arg_iterator AI = F->arg_begin();
      for (unsigned i = 0, e = TaskArgsList.size(); i != e; ++i, ++AI) {
        I.replaceUsesOfWith(TaskArgsList[i], &*AI);
      }
    }
  }

  void unpackCallToRTOfType(Module &M,
                            DenseMap<Value *, Value *> &ConstExprToInst,
                            const SmallVectorImpl<DependInfo> &DependList,
                            Function *F,
                            StringRef DepType) {
    for (const DependInfo &DI : DependList) {
      BasicBlock &Entry = F->getEntryBlock();
      Instruction &RetI = Entry.back();
      IRBuilder<> BBBuilder(&RetI);

      Value *NewBase = DI.Base;
      // Possible cases when generating the final Value for dependency base.
      // 1. ConstantExpr: We gathered this in analysis so it's safe to replace here
      // 2. GlobLVal: This will make a ConstantExpr for the BitCast, so we must to register it
      // 3. Instruction: With the BitCast we are done here
      if (isa<ConstantExpr>(NewBase))
        NewBase = ConstExprToInst[NewBase];

      NewBase = BBBuilder.CreateBitCast(NewBase, Type::getInt8PtrTy(M.getContext()));

      if (auto *CE = dyn_cast<ConstantExpr>(NewBase)) {
        if (!ConstExprToInst.count(CE)) {
          // ConstantExpr may be used more than once. Reuse the instruction
          Instruction *I = CE->getAsInstruction();
          BBBuilder.Insert(I);

          ConstExprToInst[CE] = I;
        }
      }

      SmallVector<Value *, 4> TaskDepAPICall;
      Value *Handler = &*(F->arg_end() - 1);
      TaskDepAPICall.push_back(Handler);
      TaskDepAPICall.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), DI.SymbolIndex));
      TaskDepAPICall.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(M.getContext()))); // TODO: stringify
      TaskDepAPICall.push_back(NewBase);
      assert(!(DI.Dims.size()%3));
      size_t NumDims = DI.Dims.size()/3;
      for (Value *V : DI.Dims) {
        TaskDepAPICall.push_back(V);
      }
      BBBuilder.CreateCall(MultidepFactory.getMultidepFuncCallee(M, DepType, NumDims), TaskDepAPICall);
    }
  }

  void unpackDepsCallToRT(Module &M,
                      DenseMap<Value *, Value *> &ConstExprToInst,
                      const TaskDependsInfo &TDI,
                      Function *F) {
    unpackCallToRTOfType(M, ConstExprToInst, TDI.Ins, F, "read");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.Outs, F, "write");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.Inouts, F, "readwrite");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.Concurrents, F, "concurrent");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.Commutatives, F, "commutative");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.WeakIns, F, "weak_read");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.WeakOuts, F, "weak_write");
    unpackCallToRTOfType(M, ConstExprToInst, TDI.WeakInouts, F, "weak_readwrite");
  }

  // TypeList[i] <-> NameList[i]
  // ExtraTypeList[i] <-> ExtraNameList[i]
  Function *createUnpackOlFunction(Module &M, Function &F,
                                 std::string Name,
                                 ArrayRef<Type *> TypeList,
                                 ArrayRef<StringRef> NameList,
                                 ArrayRef<Type *> ExtraTypeList,
                                 ArrayRef<StringRef> ExtraNameList) {
    Type *RetTy = Type::getVoidTy(M.getContext());

    SmallVector<Type *, 4> AggTypeList;
    AggTypeList.append(TypeList.begin(), TypeList.end());
    AggTypeList.append(ExtraTypeList.begin(), ExtraTypeList.end());

    SmallVector<StringRef, 4> AggNameList;
    AggNameList.append(NameList.begin(), NameList.end());
    AggNameList.append(ExtraNameList.begin(), ExtraNameList.end());

    FunctionType *FuncType =
      FunctionType::get(RetTy, AggTypeList, /*IsVarArgs=*/ false);

    Function *FuncVar = Function::Create(
        FuncType, GlobalValue::InternalLinkage, F.getAddressSpace(),
        Name, &M);

    // Set names for arguments.
    Function::arg_iterator AI = FuncVar->arg_begin();
    for (unsigned i = 0, e = AggNameList.size(); i != e; ++i, ++AI)
      AI->setName(AggNameList[i]);

    return FuncVar;
  }

  // Given a Outline Function assuming that task args are the first parameter, and
  // DSAInfo and VLADimsInfo, it unpacks task args in Outline and fills UnpackedList
  // with those Values, used to call Unpack Functions
  void unpackDSAsWithVLADims(Module &M, const TaskDSAInfo &DSAInfo,
                  const TaskCapturedInfo &CapturedInfo,
                  const TaskVLADimsInfo &VLADimsInfo,
                  Function *OlFunc,
                  DenseMap<Value *, size_t> &StructToIdxMap,
                  SmallVectorImpl<Value *> &UnpackedList) {
    UnpackedList.clear();

    IRBuilder<> BBBuilder(&OlFunc->getEntryBlock());
    Function::arg_iterator AI = OlFunc->arg_begin();
    Value *OlDepsFuncTaskArgs = &*AI++;
    for (Value *V : DSAInfo.Shared) {
      Value *Idx[2];
      Idx[0] = Constant::getNullValue(Type::getInt32Ty(M.getContext()));
      Idx[1] = ConstantInt::get(Type::getInt32Ty(M.getContext()), StructToIdxMap[V]);
      Value *GEP = BBBuilder.CreateGEP(
          OlDepsFuncTaskArgs, Idx, "gep_" + V->getName());
      Value *LGEP = BBBuilder.CreateLoad(GEP, "load_" + GEP->getName());
      UnpackedList.push_back(LGEP);
    }
    for (Value *V : DSAInfo.Private) {
      Value *Idx[2];
      Idx[0] = Constant::getNullValue(Type::getInt32Ty(M.getContext()));
      Idx[1] = ConstantInt::get(Type::getInt32Ty(M.getContext()), StructToIdxMap[V]);
      Value *GEP = BBBuilder.CreateGEP(
          OlDepsFuncTaskArgs, Idx, "gep_" + V->getName());

      // VLAs
      if (VLADimsInfo.count(V))
        GEP = BBBuilder.CreateLoad(GEP, "load_" + GEP->getName());

      UnpackedList.push_back(GEP);
    }
    for (Value *V : DSAInfo.Firstprivate) {
      Value *Idx[2];
      Idx[0] = Constant::getNullValue(Type::getInt32Ty(M.getContext()));
      Idx[1] = ConstantInt::get(Type::getInt32Ty(M.getContext()), StructToIdxMap[V]);
      Value *GEP = BBBuilder.CreateGEP(
          OlDepsFuncTaskArgs, Idx, "gep_" + V->getName());

      // VLAs
      if (VLADimsInfo.count(V))
        GEP = BBBuilder.CreateLoad(GEP, "load_" + GEP->getName());

      UnpackedList.push_back(GEP);
    }
    for (Value *V : CapturedInfo) {
      Value *Idx[2];
      Idx[0] = Constant::getNullValue(Type::getInt32Ty(M.getContext()));
      Idx[1] = ConstantInt::get(Type::getInt32Ty(M.getContext()), StructToIdxMap[V]);
      Value *GEP = BBBuilder.CreateGEP(
          OlDepsFuncTaskArgs, Idx, "capt_gep" + V->getName());
      Value *LGEP = BBBuilder.CreateLoad(GEP, "load_" + GEP->getName());
      UnpackedList.push_back(LGEP);
    }
  }

  // Given an Outline and Unpack Functions it unpacks DSAs in Outline
  // and builds a call to Unpack
  void olCallToUnpack(Module &M, const TaskDSAInfo &DSAInfo,
                      const TaskCapturedInfo &CapturedInfo,
                      const TaskVLADimsInfo &VLADimsInfo,
                      DenseMap<Value *, size_t> &StructToIdxMap,
                      Function *OlFunc, Function *UnpackFunc) {
    BasicBlock::Create(M.getContext(), "entry", OlFunc);
    IRBuilder<> BBBuilder(&OlFunc->getEntryBlock());

    // First arg is the nanos_task_args
    Function::arg_iterator AI = OlFunc->arg_begin();
    AI++;
    SmallVector<Value *, 4> UnpackParams;
    unpackDSAsWithVLADims(M, DSAInfo, CapturedInfo, VLADimsInfo, OlFunc, StructToIdxMap, UnpackParams);
    while (AI != OlFunc->arg_end()) {
      UnpackParams.push_back(&*AI++);
    }
    // Build TaskUnpackCall
    BBBuilder.CreateCall(UnpackFunc, UnpackParams);
    // Make BB legal with a terminator to task outline function
    BBBuilder.CreateRetVoid();
  }

  Value *computeTaskArgsVLAsExtraSizeOf(Module &M, IRBuilder<> &IRB, const TaskVLADimsInfo &VLADimsInfo) {
    Value *Sum = ConstantInt::get(IRB.getInt64Ty(), 0);
    for (auto &VLAWithDimsMap : VLADimsInfo) {
      Type *Ty = VLAWithDimsMap.first->getType()->getPointerElementType();
      unsigned SizeB = M.getDataLayout().getTypeAllocSize(Ty);
      Value *ArraySize = ConstantInt::get(IRB.getInt64Ty(), SizeB);
      for (Value *const &V : VLAWithDimsMap.second) {
        ArraySize = IRB.CreateNUWMul(ArraySize, V);
      }
      Sum = IRB.CreateNUWAdd(Sum, ArraySize);
    }
    return Sum;
  }

  StructType *createTaskArgsType(Module &M,
                                 const TaskDSAInfo &DSAInfo,
                                 const TaskCapturedInfo &CapturedInfo,
                                 const TaskVLADimsInfo &VLADimsInfo,
                                 DenseMap<Value *, size_t> &StructToIdxMap, StringRef Str) {
    // Private and Firstprivate must be stored in the struct
    // Captured values (i.e. VLA dimensions) are not pointers
    SmallVector<Type *, 4> TaskArgsMemberTy;
    size_t TaskArgsIdx = 0;
    for (Value *V : DSAInfo.Shared) {
      TaskArgsMemberTy.push_back(V->getType());
      StructToIdxMap[V] = TaskArgsIdx++;
    }
    for (Value *V : DSAInfo.Private) {
      // VLAs
      if (VLADimsInfo.count(V))
        TaskArgsMemberTy.push_back(V->getType());
      else
        TaskArgsMemberTy.push_back(V->getType()->getPointerElementType());
      StructToIdxMap[V] = TaskArgsIdx++;
    }
    for (Value *V : DSAInfo.Firstprivate) {
      // VLAs
      if (VLADimsInfo.count(V))
        TaskArgsMemberTy.push_back(V->getType());
      else
        TaskArgsMemberTy.push_back(V->getType()->getPointerElementType());
      StructToIdxMap[V] = TaskArgsIdx++;
    }
    for (Value *V : CapturedInfo) {
      assert(!V->getType()->isPointerTy() && "Captures are not pointers");
      TaskArgsMemberTy.push_back(V->getType());
      StructToIdxMap[V] = TaskArgsIdx++;
    }
    return StructType::create(M.getContext(), TaskArgsMemberTy, Str);
  }

  struct VLAAlign {
    Value *V;
    unsigned Align;
  };

  // Greater alignemt go first
  void computeVLAsAlignOrder(Module &M, SmallVectorImpl<VLAAlign> &VLAAlignsInfo, const TaskVLADimsInfo &VLADimsInfo) {
    for (const auto &VLAWithDimsMap : VLADimsInfo) {
      Value *const V = VLAWithDimsMap.first;
      Type *Ty = V->getType()->getPointerElementType();

      unsigned Align = M.getDataLayout().getPrefTypeAlignment(Ty);

      auto It = VLAAlignsInfo.begin();
      while (It != VLAAlignsInfo.end() && It->Align >= Align)
        ++It;

      VLAAlignsInfo.insert(It, {V, Align});
    }
  }

  void lowerTaskwait(const TaskwaitInfo &TwI,
                     Module &M) {
    // 1. Create Taskwait function Type
    IRBuilder<> IRB(TwI.I);
    FunctionCallee Func = M.getOrInsertFunction(
        "nanos6_taskwait", IRB.getVoidTy(), IRB.getInt8PtrTy());
    // 2. Build String
    unsigned Line = TwI.I->getDebugLoc().getLine();
    unsigned Col = TwI.I->getDebugLoc().getCol();

    std::string FileNamePlusLoc = (M.getSourceFileName()
                                   + ":" + Twine(Line)
                                   + ":" + Twine(Col)).str();
    Constant *Nanos6TaskwaitLocStr = IRB.CreateGlobalStringPtr(FileNamePlusLoc);

    // 3. Insert the call
    IRB.CreateCall(Func, {Nanos6TaskwaitLocStr});
    // 4. Remove the intrinsic
    TwI.I->eraseFromParent();
  }

  void buildFinalCondCFG(BasicBlock *EntryBB, BasicBlock *ExitBB,
                         DenseMap<BasicBlock *, BasicBlock *> &CopyBBs,
                         Function &F, Module &M) {
    ExitBB->setName("final.end");
    assert(EntryBB->getSinglePredecessor());
    BasicBlock *FinalCondBB = BasicBlock::Create(M.getContext(), "final.cond", &F);

    BasicBlock *CopyEntryBB = nullptr;
    // There is only one CopyBB that has no predecessors,
    // this is the entry
    for (auto &p : CopyBBs) {
      BasicBlock *&CopyBB = p.second;
      if (CopyBB->hasNPredecessors(0)) {
        assert(!CopyEntryBB);
        CopyEntryBB = CopyBB;
      }
    }
    assert(CopyEntryBB);
    CopyEntryBB->setName("final.then");

    // We are now just before the branch to task body
    Instruction *EntryBBTerminator = EntryBB->getSinglePredecessor()->getTerminator();

    IRBuilder<> IRB(EntryBBTerminator);

    IRB.CreateBr(FinalCondBB);
    // Remove the old branch
    EntryBBTerminator->eraseFromParent();

    IRB.SetInsertPoint(FinalCondBB);
    // if (nanos6_in_final())
    Value *Cond = IRB.CreateICmpNE(IRB.CreateCall(TaskInFinalFuncTy, {}), IRB.getInt32(0));
    IRB.CreateCondBr(Cond, CopyEntryBB, EntryBB);
  }

  void lowerTask(TaskInfo &TI,
                 Function &F,
                 size_t taskNum,
                 Module &M,
                 SmallVectorImpl<DenseMap<BasicBlock *, BasicBlock *>> &TaskCopyBBs) {

    DebugLoc DLoc = TI.Entry->getDebugLoc();
    unsigned Line = DLoc.getLine();
    unsigned Col = DLoc.getCol();
    std::string FileNamePlusLoc = (M.getSourceFileName()
                                   + ":" + Twine(Line)
                                   + ":" + Twine(Col)).str();

    Constant *Nanos6TaskLocStr = IRBuilder<>(TI.Entry).CreateGlobalStringPtr(FileNamePlusLoc);

    // 1. Split BB
    BasicBlock *EntryBB = TI.Entry->getParent();
    // EntryBB = EntryBB->splitBasicBlock(TI.Entry);

    BasicBlock *ExitBB = TI.Exit->getParent()->getNextNode();
    // Assuming well-formed BB
    // ExitBB = ExitBB->splitBasicBlock(TI.Exit->getNextNode());

    TI.Exit->eraseFromParent();
    TI.Entry->eraseFromParent();
    SetVector<BasicBlock *> TaskBBs;

    buildFinalCondCFG(EntryBB, ExitBB, TaskCopyBBs[taskNum], F, M);

    // 2. Gather BB between entry and exit (is there any function/util to do this?)
    SmallVector<BasicBlock*, 8> Worklist;
    SmallPtrSet<BasicBlock*, 8> Visited;

    Worklist.push_back(EntryBB);
    Visited.insert(EntryBB);
    TaskBBs.insert(EntryBB);
    while (!Worklist.empty()) {
      auto WIt = Worklist.begin();
      BasicBlock *BB = *WIt;
      Worklist.erase(WIt);

      for (auto It = succ_begin(BB); It != succ_end(BB); ++It) {
        if (!Visited.count(*It) && *It != ExitBB) {
          Worklist.push_back(*It);
          Visited.insert(*It);
          TaskBBs.insert(*It);
        }
      }
    }

    // Create nanos6_task_args_* START
    SmallVector<Type *, 4> TaskArgsMemberTy;
    DenseMap<Value *, size_t> TaskArgsToStructIdxMap;
    StructType *TaskArgsTy = createTaskArgsType(M, TI.DSAInfo,
                                                 TI.CapturedInfo,
                                                 TI.VLADimsInfo,
                                                 TaskArgsToStructIdxMap,
                                                 ("nanos6_task_args_" + F.getName() + Twine(taskNum)).str());
    // Create nanos6_task_args_* END

    SetVector<Value *> TaskArgsList;
    TaskArgsList.insert(TI.DSAInfo.Shared.begin(), TI.DSAInfo.Shared.end());
    TaskArgsList.insert(TI.DSAInfo.Private.begin(), TI.DSAInfo.Private.end());
    TaskArgsList.insert(TI.DSAInfo.Firstprivate.begin(), TI.DSAInfo.Firstprivate.end());
    TaskArgsList.insert(TI.CapturedInfo.begin(), TI.CapturedInfo.end());

    SmallVector<Type *, 4> TaskTypeList;
    SmallVector<StringRef, 4> TaskNameList;
    for (Value *V : TaskArgsList) {
      TaskTypeList.push_back(V->getType());
      TaskNameList.push_back(V->getName());
    }

    SmallVector<Type *, 4> TaskExtraTypeList;
    SmallVector<StringRef, 4> TaskExtraNameList;

    // void *device_env
    TaskExtraTypeList.push_back(Type::getInt8PtrTy(M.getContext()));
    TaskExtraNameList.push_back("device_env");
    // nanos6_address_translation_entry_t *address_translation_table
    TaskExtraTypeList.push_back(
      Nanos6TaskAddrTranslationEntry::getInstance(M).getType()->getPointerTo());
    TaskExtraNameList.push_back("address_translation_table");

    // nanos6_unpacked_task_region_* START
    // CodeExtractor will create a entry block for us
    Function *UnpackTaskFuncVar
      = createUnpackOlFunction(M, F,
                               ("nanos6_unpacked_task_region_" + F.getName() + Twine(taskNum)).str(),
                               TaskTypeList, TaskNameList,
                               TaskExtraTypeList, TaskExtraNameList);

    // nanos6_unpacked_task_region_* END

    // nanos6_ol_task_region_* START
    Function *OlTaskFuncVar
      = createUnpackOlFunction(M, F,
                               ("nanos6_ol_task_region_" + F.getName() + Twine(taskNum)).str(),
                               {TaskArgsTy->getPointerTo()}, {"task_args"},
                               TaskExtraTypeList, TaskExtraNameList);

    olCallToUnpack(M, TI.DSAInfo, TI.CapturedInfo, TI.VLADimsInfo, TaskArgsToStructIdxMap, OlTaskFuncVar, UnpackTaskFuncVar);

    // nanos6_ol_task_region_* END

    // nanos6_unpacked_deps_* START
    TaskExtraTypeList.clear();
    TaskExtraNameList.clear();
    // nanos6_loop_bounds_t *const loop_bounds
    TaskExtraTypeList.push_back(Type::getInt8PtrTy(M.getContext()));
    TaskExtraNameList.push_back("loop_bounds");
    // void *handler
    TaskExtraTypeList.push_back(Type::getInt8PtrTy(M.getContext()));
    TaskExtraNameList.push_back("handler");

    Function *UnpackDepsFuncVar
      = createUnpackOlFunction(M, F,
                               ("nanos6_unpacked_deps_" + F.getName() + Twine(taskNum)).str(),
                               TaskTypeList, TaskNameList,
                               TaskExtraTypeList, TaskExtraNameList);
    unpackDepsAndRewrite(M, TI.DependsInfo, UnpackDepsFuncVar, TaskArgsList.getArrayRef());

    // nanos6_unpacked_deps_* END

    // nanos6_ol_deps_* START

    Function *OlDepsFuncVar
      = createUnpackOlFunction(M, F,
                               ("nanos6_ol_deps_" + F.getName() + Twine(taskNum)).str(),
                               {TaskArgsTy->getPointerTo()}, {"task_args"},
                               TaskExtraTypeList, TaskExtraNameList);
    olCallToUnpack(M, TI.DSAInfo, TI.CapturedInfo, TI.VLADimsInfo, TaskArgsToStructIdxMap, OlDepsFuncVar, UnpackDepsFuncVar);

    // nanos6_ol_deps_* END

    Function *OlConstraintsFuncVar = nullptr;
    if (TI.Cost) {
      // nanos6_unpacked_constraints_* START
      TaskExtraTypeList.clear();
      TaskExtraNameList.clear();
      // nanos6_task_constraints_t *constraints
      TaskExtraTypeList.push_back(Nanos6TaskConstraints::getInstance(M).getType()->getPointerTo());
      TaskExtraNameList.push_back("constraints");

      Function *UnpackConstraintsFuncVar = createUnpackOlFunction(M, F,
                                 ("nanos6_unpacked_constraints_" + F.getName() + Twine(taskNum)).str(),
                                 TaskTypeList, TaskNameList,
                                 TaskExtraTypeList, TaskExtraNameList);
      unpackCostAndRewrite(M, TI.Cost, UnpackConstraintsFuncVar, TaskArgsList.getArrayRef());
      // nanos6_unpacked_constraints_* END

      // nanos6_ol_constraints_* START

      OlConstraintsFuncVar
        = createUnpackOlFunction(M, F,
                                 ("nanos6_ol_constraints_" + F.getName() + Twine(taskNum)).str(),
                                 {TaskArgsTy->getPointerTo()}, {"task_args"},
                                 TaskExtraTypeList, TaskExtraNameList);
      olCallToUnpack(M, TI.DSAInfo, TI.CapturedInfo, TI.VLADimsInfo, TaskArgsToStructIdxMap, OlConstraintsFuncVar, UnpackConstraintsFuncVar);

      // nanos6_ol_constraints_* END
    }

    Function *OlPriorityFuncVar = nullptr;
    if (TI.Priority) {
      // nanos6_unpacked_priority_* START
      TaskExtraTypeList.clear();
      TaskExtraNameList.clear();
      // nanos6_priority_t *priority
      // long int *priority
      TaskExtraTypeList.push_back(Type::getInt64Ty(M.getContext())->getPointerTo());
      TaskExtraNameList.push_back("priority");

      Function *UnpackPriorityFuncVar = createUnpackOlFunction(M, F,
                                 ("nanos6_unpacked_priority_" + F.getName() + Twine(taskNum)).str(),
                                 TaskTypeList, TaskNameList,
                                 TaskExtraTypeList, TaskExtraNameList);
      unpackPriorityAndRewrite(M, TI.Priority, UnpackPriorityFuncVar, TaskArgsList.getArrayRef());
      // nanos6_unpacked_priority_* END

      // nanos6_ol_priority_* START

      OlPriorityFuncVar
        = createUnpackOlFunction(M, F,
                                 ("nanos6_ol_priority_" + F.getName() + Twine(taskNum)).str(),
                                 {TaskArgsTy->getPointerTo()}, {"task_args"},
                                 TaskExtraTypeList, TaskExtraNameList);
      olCallToUnpack(M, TI.DSAInfo, TI.CapturedInfo, TI.VLADimsInfo, TaskArgsToStructIdxMap, OlPriorityFuncVar, UnpackPriorityFuncVar);

      // nanos6_ol_priority_* END
    }

    // 3. Create Nanos6 task data structures info
    Constant *TaskInvInfoVar = M.getOrInsertGlobal(("task_invocation_info_" + F.getName() + Twine(taskNum)).str(),
                                      Nanos6TaskInvInfo::getInstance(M).getType(),
                                      [&M, &F, &Nanos6TaskLocStr, &taskNum] {
      GlobalVariable *GV = new GlobalVariable(M, Nanos6TaskInvInfo::getInstance(M).getType(),
                                /*isConstant=*/true,
                                GlobalVariable::InternalLinkage,
                                ConstantStruct::get(Nanos6TaskInvInfo::getInstance(M).getType(),
                                                    Nanos6TaskLocStr),
                                ("task_invocation_info_" + F.getName() + Twine(taskNum)).str());
      GV->setAlignment(64);
      return GV;
    });

    Constant *TaskImplInfoVar = M.getOrInsertGlobal(("implementations_var_" + F.getName() + Twine(taskNum)).str(),
                                      ArrayType::get(Nanos6TaskImplInfo::getInstance(M).getType(), 1),
                                      [&M, &F, &OlTaskFuncVar,
                                       &OlConstraintsFuncVar, &Nanos6TaskLocStr,
                                       &taskNum] {
      GlobalVariable *GV = new GlobalVariable(M, ArrayType::get(Nanos6TaskImplInfo::getInstance(M).getType(), 1),
                                /*isConstant=*/true,
                                GlobalVariable::InternalLinkage,
                                ConstantArray::get(ArrayType::get(Nanos6TaskImplInfo::getInstance(M).getType(), 1), // TODO: More than one implementations?
                                                   ConstantStruct::get(Nanos6TaskImplInfo::getInstance(M).getType(),
                                                                       ConstantInt::get(Nanos6TaskImplInfo::getInstance(M).getType()->getElementType(0), 0),
                                                                       ConstantExpr::getPointerCast(OlTaskFuncVar, Nanos6TaskImplInfo::getInstance(M).getType()->getElementType(1)),
                                                                       ConstantPointerNull::get(cast<PointerType>(Nanos6TaskImplInfo::getInstance(M).getType()->getElementType(2))),
                                                                       OlConstraintsFuncVar
                                                                         ? ConstantExpr::getPointerCast(OlConstraintsFuncVar,
                                                                                                        Nanos6TaskImplInfo::getInstance(M).getType()->getElementType(3))
                                                                         : ConstantPointerNull::get(cast<PointerType>(Nanos6TaskImplInfo::getInstance(M).getType()->getElementType(3))),
                                                                       Nanos6TaskLocStr,
                                                                       ConstantPointerNull::get(cast<PointerType>(Nanos6TaskImplInfo::getInstance(M).getType()->getElementType(5))))),
                                ("implementations_var_" + F.getName() + Twine(taskNum)).str());

      GV->setAlignment(64);
      return GV;
    });

    Constant *TaskInfoVar = M.getOrInsertGlobal(("task_info_var_" + F.getName() + Twine(taskNum)).str(),
                                      Nanos6TaskInfo::getInstance(M).getType(),
                                      [&M, &F, &TI, &OlDepsFuncVar,
                                       &OlPriorityFuncVar,
                                       &TaskImplInfoVar, &taskNum] {
      GlobalVariable *GV = new GlobalVariable(M, Nanos6TaskInfo::getInstance(M).getType(),
                                /*isConstant=*/true,
                                GlobalVariable::InternalLinkage,
                                ConstantStruct::get(Nanos6TaskInfo::getInstance(M).getType(),
                                                    // TODO: Add support for devices
                                                    ConstantInt::get(Nanos6TaskInfo::getInstance(M).getType()->getElementType(0), TI.DependsInfo.NumSymbols),
                                                    ConstantExpr::getPointerCast(OlDepsFuncVar, Nanos6TaskInfo::getInstance(M).getType()->getElementType(1)),
                                                    OlPriorityFuncVar
                                                      ? ConstantExpr::getPointerCast(OlPriorityFuncVar,
                                                                                     Nanos6TaskInfo::getInstance(M).getType()->getElementType(2))
                                                      : ConstantPointerNull::get(cast<PointerType>(Nanos6TaskInfo::getInstance(M).getType()->getElementType(2))),
                                                    ConstantPointerNull::get(cast<PointerType>(Nanos6TaskInfo::getInstance(M).getType()->getElementType(3))),
                                                    ConstantInt::get(Nanos6TaskInfo::getInstance(M).getType()->getElementType(4), 1),
                                                    ConstantExpr::getPointerCast(TaskImplInfoVar, Nanos6TaskInfo::getInstance(M).getType()->getElementType(5)),
                                                    ConstantPointerNull::get(cast<PointerType>(Nanos6TaskInfo::getInstance(M).getType()->getElementType(6))),
                                                    ConstantPointerNull::get(cast<PointerType>(Nanos6TaskInfo::getInstance(M).getType()->getElementType(7))),
                                                    ConstantPointerNull::get(cast<PointerType>(Nanos6TaskInfo::getInstance(M).getType()->getElementType(8))),
                                                    ConstantPointerNull::get(cast<PointerType>(Nanos6TaskInfo::getInstance(M).getType()->getElementType(9)))),
                                ("task_info_var_" + F.getName() + Twine(taskNum)).str());

      GV->setAlignment(64);
      return GV;
    });

    auto rewriteUsesBrAndGetOmpSsUnpackFunc
      = [&UnpackTaskFuncVar, &TaskArgsList](BasicBlock *header,
                                            BasicBlock *newRootNode,
                                            BasicBlock *newHeader,
                                            Function *oldFunction,
                                            const SetVector<BasicBlock *> &Blocks) {

      UnpackTaskFuncVar->getBasicBlockList().push_back(newRootNode);

      // Create an iterator to name all of the arguments we inserted.
      Function::arg_iterator AI = UnpackTaskFuncVar->arg_begin();
      // Rewrite all users of the TaskArgsList in the extracted region to use the
      // arguments (or appropriate addressing into struct) instead.
      for (unsigned i = 0, e = TaskArgsList.size(); i != e; ++i) {
        Value *RewriteVal = &*AI++;

        std::vector<User *> Users(TaskArgsList[i]->user_begin(), TaskArgsList[i]->user_end());
        for (User *use : Users)
          if (Instruction *inst = dyn_cast<Instruction>(use))
            if (Blocks.count(inst->getParent()))
              inst->replaceUsesOfWith(TaskArgsList[i], RewriteVal);
      }

      // Rewrite branches from basic blocks outside of the task region to blocks
      // inside the region to use the new label (newHeader) since the task region
      // will be outlined
      std::vector<User *> Users(header->user_begin(), header->user_end());
      for (unsigned i = 0, e = Users.size(); i != e; ++i)
        // The BasicBlock which contains the branch is not in the region
        // modify the branch target to a new block
        if (Instruction *I = dyn_cast<Instruction>(Users[i]))
          if (I->isTerminator() && !Blocks.count(I->getParent()) &&
              I->getParent()->getParent() == oldFunction)
            I->replaceUsesOfWith(header, newHeader);

      return UnpackTaskFuncVar;
    };
    auto emitOmpSsCaptureAndSubmitTask
      = [this, &M, &DLoc, &TaskArgsTy,
         &TI, &TaskArgsToStructIdxMap,
         &TaskInfoVar, &TaskInvInfoVar](Function *newFunction,
                                        BasicBlock *codeReplacer,
                                        const SetVector<BasicBlock *> &Blocks) {

      IRBuilder<> IRB(codeReplacer);
      // Set debug info from the task entry to all instructions
      IRB.SetCurrentDebugLocation(DLoc);

      AllocaInst *TaskArgsVar = IRB.CreateAlloca(TaskArgsTy->getPointerTo());
      Value *TaskArgsVarCast = IRB.CreateBitCast(TaskArgsVar, IRB.getInt8PtrTy()->getPointerTo());
      // TaskFlagsVar = !If << 1 | Final
      Value *TaskFlagsVar = ConstantInt::get(IRB.getInt64Ty(), 0);
      if (TI.Final) {
        TaskFlagsVar =
          IRB.CreateOr(
            TaskFlagsVar,
            IRB.CreateZExt(TI.Final,
                           IRB.getInt64Ty()));
      }
      if (TI.If) {
        TaskFlagsVar =
          IRB.CreateOr(
            TaskFlagsVar,
            IRB.CreateShl(
              IRB.CreateZExt(
                IRB.CreateICmpEQ(TI.If, IRB.getFalse()),
                IRB.getInt64Ty()),
                1));
      }
      Value *TaskPtrVar = IRB.CreateAlloca(IRB.getInt8PtrTy());

      Value *TaskArgsStructSizeOf = ConstantInt::get(IRB.getInt64Ty(), M.getDataLayout().getTypeAllocSize(TaskArgsTy));

      // TODO: this forces an alignment of 16 for VLAs
      {
        const int ALIGN = 16;
        TaskArgsStructSizeOf =
          IRB.CreateNUWAdd(TaskArgsStructSizeOf,
                           ConstantInt::get(IRB.getInt64Ty(), ALIGN - 1));
        TaskArgsStructSizeOf =
          IRB.CreateAnd(TaskArgsStructSizeOf,
                        IRB.CreateNot(ConstantInt::get(IRB.getInt64Ty(), ALIGN - 1)));
      }

      Value *TaskArgsVLAsExtraSizeOf = computeTaskArgsVLAsExtraSizeOf(M, IRB, TI.VLADimsInfo);
      Value *TaskArgsSizeOf = IRB.CreateNUWAdd(TaskArgsStructSizeOf, TaskArgsVLAsExtraSizeOf);
      IRB.CreateCall(CreateTaskFuncTy, {TaskInfoVar,
                                  TaskInvInfoVar,
                                  TaskArgsSizeOf,
                                  TaskArgsVarCast,
                                  TaskPtrVar,
                                  TaskFlagsVar,
                                  ConstantInt::get(IRB.getInt64Ty(),
                                                   TI.DependsInfo.Ins.size()
                                                   + TI.DependsInfo.Outs.size())}); // TaskNumDepsVar;

      // DSA capture
      Value *TaskArgsVarL = IRB.CreateLoad(TaskArgsVar);

      Value *TaskArgsVarLi8 = IRB.CreateBitCast(TaskArgsVarL, IRB.getInt8PtrTy());
      Value *TaskArgsVarLi8IdxGEP = IRB.CreateGEP(TaskArgsVarLi8, TaskArgsStructSizeOf, "args_end");

      SmallVector<VLAAlign, 2> VLAAlignsInfo;
      computeVLAsAlignOrder(M, VLAAlignsInfo, TI.VLADimsInfo);

      // First point VLAs to its according space in task args
      for (const VLAAlign& VAlign : VLAAlignsInfo) {
        Value *const V = VAlign.V;
        size_t Align = VAlign.Align;

        Type *Ty = V->getType()->getPointerElementType();

        Value *Idx[2];
        Idx[0] = Constant::getNullValue(IRB.getInt32Ty());
        Idx[1] = ConstantInt::get(IRB.getInt32Ty(), TaskArgsToStructIdxMap[V]);
        Value *GEP = IRB.CreateGEP(
            TaskArgsVarL, Idx, "gep_" + V->getName());

        // Point VLA in task args to an aligned position of the extra space allocated
        Value *GEPi8 = IRB.CreateBitCast(GEP, IRB.getInt8PtrTy()->getPointerTo());
        IRB.CreateAlignedStore(TaskArgsVarLi8IdxGEP, GEPi8, Align);
        // Skip current VLA size
        unsigned SizeB = M.getDataLayout().getTypeAllocSize(Ty);
        Value *VLASize = ConstantInt::get(IRB.getInt64Ty(), SizeB);
        for (Value *const &Dim : TI.VLADimsInfo[V])
          VLASize = IRB.CreateNUWMul(VLASize, Dim);
        TaskArgsVarLi8IdxGEP = IRB.CreateGEP(TaskArgsVarLi8IdxGEP, VLASize);
      }

      for (Value *V : TI.DSAInfo.Shared) {
        Value *Idx[2];
        Idx[0] = Constant::getNullValue(IRB.getInt32Ty());
        Idx[1] = ConstantInt::get(IRB.getInt32Ty(), TaskArgsToStructIdxMap[V]);
        Value *GEP = IRB.CreateGEP(
            TaskArgsVarL, Idx, "gep_" + V->getName());
        IRB.CreateStore(V, GEP);
      }
      for (Value *V : TI.DSAInfo.Private) {
        // Call custom constructor generated in clang in non-pods
        // Leave pods unititialized
        auto It = TI.NonPODsInfo.Inits.find(V);
        if (It != TI.NonPODsInfo.Inits.end()) {
          Type *Ty = V->getType()->getPointerElementType();
          // Compute num elements
          Value *NSize = ConstantInt::get(IRB.getInt64Ty(), 1);
          if (isa<ArrayType>(Ty)) {
            while (ArrayType *ArrTy = dyn_cast<ArrayType>(Ty)) {
              // Constant array
              Value *NumElems = ConstantInt::get(IRB.getInt64Ty(),
                                                 ArrTy->getNumElements());
              NSize = IRB.CreateNUWMul(NSize, NumElems);
              Ty = ArrTy->getElementType();
            }
          } else if (TI.VLADimsInfo.count(V)) {
            for (Value *const &Dim : TI.VLADimsInfo[V])
              NSize = IRB.CreateNUWMul(NSize, Dim);
          }

          Value *Idx[2];
          Idx[0] = Constant::getNullValue(IRB.getInt32Ty());
          Idx[1] = ConstantInt::get(IRB.getInt32Ty(), TaskArgsToStructIdxMap[V]);
          Value *GEP = IRB.CreateGEP(
              TaskArgsVarL, Idx, "gep_" + V->getName());

          // VLAs
          if (TI.VLADimsInfo.count(V))
            GEP = IRB.CreateLoad(GEP);

          // Regular arrays have types like [10 x %struct.S]*
          // Cast to %struct.S*
          GEP = IRB.CreateBitCast(GEP, Ty->getPointerTo());

          IRB.CreateCall(It->second, ArrayRef<Value*>{GEP, NSize});
        }
      }
      for (Value *V : TI.DSAInfo.Firstprivate) {
        Type *Ty = V->getType()->getPointerElementType();
        unsigned Align = M.getDataLayout().getPrefTypeAlignment(Ty);

        // Compute num elements
        Value *NSize = ConstantInt::get(IRB.getInt64Ty(), 1);
        if (isa<ArrayType>(Ty)) {
          while (ArrayType *ArrTy = dyn_cast<ArrayType>(Ty)) {
            // Constant array
            Value *NumElems = ConstantInt::get(IRB.getInt64Ty(),
                                               ArrTy->getNumElements());
            NSize = IRB.CreateNUWMul(NSize, NumElems);
            Ty = ArrTy->getElementType();
          }
        } else if (TI.VLADimsInfo.count(V)) {
          for (Value *const &Dim : TI.VLADimsInfo[V])
            NSize = IRB.CreateNUWMul(NSize, Dim);
        }

        // call custom copy constructor generated in clang in non-pods
        // do a memcpy if pod
        Value *Idx[2];
        Idx[0] = Constant::getNullValue(IRB.getInt32Ty());
        Idx[1] = ConstantInt::get(IRB.getInt32Ty(), TaskArgsToStructIdxMap[V]);
        Value *GEP = IRB.CreateGEP(
            TaskArgsVarL, Idx, "gep_" + V->getName());

        // VLAs
        if (TI.VLADimsInfo.count(V))
          GEP = IRB.CreateLoad(GEP);

        auto It = TI.NonPODsInfo.Copies.find(V);
        if (It != TI.NonPODsInfo.Copies.end()) {
          // Non-POD

          // Regular arrays have types like [10 x %struct.S]*
          // Cast to %struct.S*
          GEP = IRB.CreateBitCast(GEP, Ty->getPointerTo());
          V = IRB.CreateBitCast(V, Ty->getPointerTo());

          IRB.CreateCall(It->second, ArrayRef<Value*>{/*Src=*/V, /*Dst=*/GEP, NSize});
        } else {
          unsigned SizeB = M.getDataLayout().getTypeAllocSize(Ty);
          Value *NSizeB = IRB.CreateNUWMul(NSize, ConstantInt::get(IRB.getInt64Ty(), SizeB));
          IRB.CreateMemCpy(GEP, Align, V, Align, NSizeB);
        }
      }
      for (Value *V : TI.CapturedInfo) {
        Value *Idx[2];
        Idx[0] = Constant::getNullValue(IRB.getInt32Ty());
        Idx[1] = ConstantInt::get(IRB.getInt32Ty(), TaskArgsToStructIdxMap[V]);
        Value *GEP = IRB.CreateGEP(
            TaskArgsVarL, Idx, "capt_gep_" + V->getName());
        IRB.CreateStore(V, GEP);
      }

      Value *TaskPtrVarL = IRB.CreateLoad(TaskPtrVar);
      CallInst *TaskSubmitFuncCall = IRB.CreateCall(TaskSubmitFuncTy, TaskPtrVarL);

      // Add a branch to the next basic block after the task region
      // and replace the terminator that exits the task region
      // Since this is a single entry single exit region this should
      // be done once.
      Instruction *OldT = nullptr;
      for (BasicBlock *Block : Blocks) {
        Instruction *TI = Block->getTerminator();
        for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i)
          if (!Blocks.count(TI->getSuccessor(i))) {
            assert(!OldT && "More than one exit in task code");

            BasicBlock *OldTarget = TI->getSuccessor(i);

            // Create branch to next BB after the task region
            IRB.CreateBr(OldTarget);

            IRBuilder<> BNewTerminatorI(TI);
            BNewTerminatorI.CreateRetVoid();

            OldT = TI;
          }
      }
      OldT->eraseFromParent();

      return TaskSubmitFuncCall;
    };

    // 4. Extract region the way we want
    CodeExtractorAnalysisCache CEAC(F);
    CodeExtractor CE(TaskBBs.getArrayRef(), rewriteUsesBrAndGetOmpSsUnpackFunc, emitOmpSsCaptureAndSubmitTask);
    CE.extractCodeRegion(CEAC);

    // Call Dtors
    // Find 'ret' instr.
    // TODO: We assume there will be only one
    Instruction *RetI = nullptr;
    for (auto I = inst_begin(UnpackTaskFuncVar); I != inst_end(UnpackTaskFuncVar); ++I) {
      if (isa<ReturnInst>(*I)) {
        RetI = &*I;
        break;
      }
    }
    assert(RetI && "UnpackTaskFunc does not have a terminator 'ret'");

    IRBuilder<> IRB(RetI);
    for (Value *V : TI.DSAInfo.Private) {
      // Call custom destructor in clang in non-pods
      auto It = TI.NonPODsInfo.Deinits.find(V);
      if (It != TI.NonPODsInfo.Deinits.end()) {
        Type *Ty = V->getType()->getPointerElementType();
        // Compute num elements
        Value *NSize = ConstantInt::get(IRB.getInt64Ty(), 1);
        if (isa<ArrayType>(Ty)) {
          while (ArrayType *ArrTy = dyn_cast<ArrayType>(Ty)) {
            // Constant array
            Value *NumElems = ConstantInt::get(IRB.getInt64Ty(),
                                               ArrTy->getNumElements());
            NSize = IRB.CreateNUWMul(NSize, NumElems);
            Ty = ArrTy->getElementType();
          }
        } else if (TI.VLADimsInfo.count(V)) {
          for (Value *const &Dim : TI.VLADimsInfo[V])
            NSize = IRB.CreateNUWMul(NSize, UnpackTaskFuncVar->getArg(TaskArgsToStructIdxMap[Dim]));
        }

        // Regular arrays have types like [10 x %struct.S]*
        // Cast to %struct.S*
        Value *FArg = IRB.CreateBitCast(UnpackTaskFuncVar->getArg(TaskArgsToStructIdxMap[V]), Ty->getPointerTo());

        IRB.CreateCall(It->second, ArrayRef<Value*>{FArg, NSize});
      }
    }
    for (Value *V : TI.DSAInfo.Firstprivate) {
      // Call custom destructor in clang in non-pods
      auto It = TI.NonPODsInfo.Deinits.find(V);
      if (It != TI.NonPODsInfo.Deinits.end()) {
        Type *Ty = V->getType()->getPointerElementType();
        // Compute num elements
        Value *NSize = ConstantInt::get(IRB.getInt64Ty(), 1);
        if (isa<ArrayType>(Ty)) {
          while (ArrayType *ArrTy = dyn_cast<ArrayType>(Ty)) {
            // Constant array
            Value *NumElems = ConstantInt::get(IRB.getInt64Ty(),
                                               ArrTy->getNumElements());
            NSize = IRB.CreateNUWMul(NSize, NumElems);
            Ty = ArrTy->getElementType();
          }
        } else if (TI.VLADimsInfo.count(V)) {
          for (Value *const &Dim : TI.VLADimsInfo[V])
            NSize = IRB.CreateNUWMul(NSize, UnpackTaskFuncVar->getArg(TaskArgsToStructIdxMap[Dim]));
        }

        // Regular arrays have types like [10 x %struct.S]*
        // Cast to %struct.S*
        Value *FArg = IRB.CreateBitCast(UnpackTaskFuncVar->getArg(TaskArgsToStructIdxMap[V]), Ty->getPointerTo());

        IRB.CreateCall(It->second, ArrayRef<Value*>{FArg, NSize});
      }
    }
  }

  void buildNanos6Types(Module &M) {

    // Create function types
    // nanos6_create_task
    // nanos6_submit_task
    // nanos6_in_final

    CreateTaskFuncTy = M.getOrInsertFunction("nanos6_create_task",
        Type::getVoidTy(M.getContext()),
        Nanos6TaskInfo::getInstance(M).getType()->getPointerTo(),   // nanos6_task_info_t *task_info
        Nanos6TaskInvInfo::getInstance(M).getType()->getPointerTo(),// nanos6_task_invocation_info_t *task_invocation_info
        Type::getInt64Ty(M.getContext()),                         // size_t args_lock_size
        Type::getInt8PtrTy(M.getContext())->getPointerTo(),       // void **args_block_pointer
        Type::getInt8PtrTy(M.getContext())->getPointerTo(),       // void **task_pointer
        Type::getInt64Ty(M.getContext()),                         // size_t flags
        Type::getInt64Ty(M.getContext())                          // size_t num_deps
    );

    TaskSubmitFuncTy = M.getOrInsertFunction("nanos6_submit_task",
        Type::getVoidTy(M.getContext()),
        Type::getInt8PtrTy(M.getContext()) // void *task
    );

    TaskInFinalFuncTy = M.getOrInsertFunction("nanos6_in_final",
        Type::getInt32Ty(M.getContext())
    );
  }

  bool runOnModule(Module &M) override {
    if (skipModule(M))
      return false;

    // Keep all the functions before start outlining
    // to avoid analize them.
    SmallVector<Function *, 4> Functs;
    for (auto &F : M) {
      // Nothing to do for declarations.
      if (F.isDeclaration() || F.empty())
        continue;

      Functs.push_back(&F);
    }

    if (!Initialized) {
      Initialized = true;
      buildNanos6Types(M);
    }

    for (auto *F : Functs) {
      SmallVector<DenseMap<BasicBlock *, BasicBlock *>, 4> TaskCopyBBs;

      FunctionInfo &FI = getAnalysis<OmpSsRegionAnalysisPass>(*F).getFuncInfo();
      TaskFunctionInfo &TFI = FI.TaskFuncInfo;
      TaskwaitFunctionInfo &TwFI = FI.TaskwaitFuncInfo;

      // First sweep to clone BBs
      for (TaskInfo &TI : TFI.PostOrder) {
        // 1. Split BB
        BasicBlock *EntryBB = TI.Entry->getParent();
        EntryBB = EntryBB->splitBasicBlock(TI.Entry);

        BasicBlock *ExitBB = TI.Exit->getParent();
        // Assuming well-formed BB
        ExitBB = ExitBB->splitBasicBlock(TI.Exit->getNextNode());
        // TI.Exit->eraseFromParent();

        // 2. Gather BB between entry and exit (is there any function/util to do this?)
        SmallVector<BasicBlock*, 8> Worklist;
        SmallPtrSet<BasicBlock*, 8> Visited;
        SetVector<BasicBlock *> TaskBBs;

        Worklist.push_back(EntryBB);
        Visited.insert(EntryBB);
        TaskBBs.insert(EntryBB);
        while (!Worklist.empty()) {
          auto WIt = Worklist.begin();
          BasicBlock *BB = *WIt;
          Worklist.erase(WIt);

          for (auto It = succ_begin(BB); It != succ_end(BB); ++It) {
            if (!Visited.count(*It) && *It != ExitBB) {
              Worklist.push_back(*It);
              Visited.insert(*It);
              TaskBBs.insert(*It);
            }
          }
        }
        DenseMap<BasicBlock *, BasicBlock *> CopyBBs;
        ValueToValueMapTy VMap;
        // 1. Clone BBs
        for (BasicBlock *BB : TaskBBs) {
          BasicBlock *CopyBB = CloneBasicBlock(BB, VMap, ".clone", F);
          CopyBBs[BB] = CopyBB;
          // Map the BBs too
          VMap[BB] = CopyBB;
        }
        // 2. Rewrite ops and branches to cloned ones.
        //    Intrinsic exit is mapped to the original entry, so before removing it
        //    we must to map it to the cloned entry.
        for (auto &p : CopyBBs) {
          BasicBlock *& CopyBB = p.second;
          for (BasicBlock::iterator II = CopyBB->begin(), E = CopyBB->end(); II != E;) {
            Instruction &I = *II++;
            // Remove OmpSs-2 intrinsics before, since RemapInstruction will crash.
            // This happers because VMap has the map <IEntry, IcloneEntry>, we erase IcloneEntry
            // but the map is kept. When remapping IcloneExit that entry is used...
            if (auto *IIntr = dyn_cast<IntrinsicInst>(&I)) {
              Intrinsic::ID IID = IIntr->getIntrinsicID();
              if (IID == Intrinsic::directive_region_entry
                  || IID == Intrinsic::directive_region_exit
                  || IID == Intrinsic::directive_marker) {
                if (!IIntr->use_empty())
                  IIntr->replaceAllUsesWith(UndefValue::get(IIntr->getType()));

                assert(IIntr->getParent() &&
                       "BB containing IIntr deleted unexpectedly!");
                IIntr->eraseFromParent();
                continue;
              }
            }
            RemapInstruction(&I, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
          }
        }
        TaskCopyBBs.push_back(CopyBBs);
      }

      for (TaskwaitInfo& TwI : TwFI.PostOrder) {
        lowerTaskwait(TwI, M);
      }

      size_t taskNum = 0;
      for (TaskInfo &TI : TFI.PostOrder) {
        lowerTask(TI, *F, taskNum++, M, TaskCopyBBs);
      }

    }
    return true;
  }

  StringRef getPassName() const override { return "Nanos6 Lowering"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<OmpSsRegionAnalysisPass>();
  }

};

}

char OmpSs::ID = 0;

ModulePass *llvm::createOmpSsPass() {
  return new OmpSs();
}

void LLVMOmpSsPass(LLVMPassManagerRef PM) {
  unwrap(PM)->add(createOmpSsPass());
}

INITIALIZE_PASS_BEGIN(OmpSs, "ompss-2",
                "Transforms OmpSs-2 llvm.directive.region intrinsics", false, false)
INITIALIZE_PASS_DEPENDENCY(OmpSsRegionAnalysisPass)
INITIALIZE_PASS_END(OmpSs, "ompss-2",
                "Transforms OmpSs-2 llvm.directive.region intrinsics", false, false)
