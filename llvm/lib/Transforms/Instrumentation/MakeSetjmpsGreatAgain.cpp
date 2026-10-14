//===- MakeSetjmpsGreatAgan.cpp - make setjmps great again ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of Fil-C. https://fil-c.org/
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation/MakeSetjmpsGreatAgain.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CycleInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;

namespace {

constexpr bool verbose = false;

struct MakeSetjmpsGreatAgain {
  MakeSetjmpsGreatAgain(Module &M): M(M) {}

  Module &M;
  LLVMContext &C = M.getContext();

  bool isSetjmpCall(Instruction &I) {
    if (CallBase *CI = dyn_cast<CallBase>(&I))
      return CI->hasFnAttr(Attribute::ReturnsTwice);
    return false;
  }

  void run(Function &F) {
    if (F.isDeclaration())
      return;

    if (!F.callsFunctionThatReturnsTwice())
      return;

    // We only care about setjmps that might be in a loop and variables live across the setjmp that
    // might be in a loop.
    //
    // FIXME: I think LLVM has some shit that lets you avoid executing analyses directly or whatever.
    CycleInfo CI;
    CI.compute(F);
    
    // Figure out what's live across setjmps.
    std::unordered_map<BasicBlock*, std::unordered_set<Value*>> LiveAtTail;

    bool Changed = true;

    auto Execute = [&] (std::unordered_set<Value*> &Live, Instruction &I) {
      Live.erase(&I);
      
      if (PHINode* Phi = dyn_cast<PHINode>(&I)) {
        for (unsigned Index = Phi->getNumIncomingValues(); Index--;) {
          Value* V = Phi->getIncomingValue(Index);
          BasicBlock* PBB = Phi->getIncomingBlock(Index);
          Changed |= LiveAtTail[PBB].insert(V).second;
        }
        return;
      }
      
      for (Value* V : I.operand_values())
        Live.insert(V);
    };

    std::vector<BasicBlock*> Blocks;
    for (BasicBlock& BB : F)
      Blocks.push_back(&BB);
    
    while (Changed) {
      Changed = false;

      for (size_t Index = Blocks.size(); Index--;) {
        BasicBlock *BB = Blocks[Index];
        std::unordered_set<Value*> Live = LiveAtTail[BB];

        for (auto It = BB->rbegin(); It != BB->rend(); ++It)
          Execute(Live, *It);
      
        for (BasicBlock* PBB : predecessors(BB)) {
          for (Value* LV : Live)
            Changed |= LiveAtTail[PBB].insert(LV).second;
        }
      }
    }

    std::unordered_set<Instruction*> LiveAcrossSetjmp;
    std::vector<Instruction*> Setjmps;
    
    for (BasicBlock &BB : F) {
      std::unordered_set<Value*> Live = LiveAtTail[&BB];
      for (auto It = BB.rbegin(); It != BB.rend(); ++It) {
        if (isSetjmpCall(*It) && CI.getCycle((*It).getParent())) {
          Setjmps.push_back(&*It);
          for (Value *V : Live) {
            if (Instruction *I = dyn_cast<Instruction>(V)) {
              if (CI.getCycle(I->getParent()))
                LiveAcrossSetjmp.insert(I);
            }
          }
        }
        Execute(Live, *It);
      }
    }

    assert(!Changed);

    std::vector<AllocaInst*> AllocaVarAcrossSetjmp;
    std::vector<AllocaInst*> AllocaSaveAcrossSetjmp;
    for (Instruction *I : LiveAcrossSetjmp) {
      AllocaInst* AI = DemoteRegToStack(*I);
      assert(!AI->isArrayAllocation());
      AllocaInst* OtherAI = cast<AllocaInst>(AI->clone());
      OtherAI->insertBefore(AI);
      AllocaVarAcrossSetjmp.push_back(AI);
      AllocaSaveAcrossSetjmp.push_back(OtherAI);
    }

    if (verbose)
      errs() << "Function after reg2mem: " << F << "\n";
    
    for (Instruction* Setjmp : Setjmps) {
      Instruction* AfterSetjmp = Setjmp->getNextNode();
      for (unsigned Index = AllocaVarAcrossSetjmp.size(); Index--;) {
        AllocaInst* Var = AllocaVarAcrossSetjmp[Index];
        AllocaInst* Save = AllocaSaveAcrossSetjmp[Index];
        new StoreInst(
          new LoadInst(Var->getAllocatedType(), Var, "msga_preload", Setjmp),
          Save, /*isVolatile=*/true, Setjmp);
        
        new StoreInst(
          new LoadInst(Var->getAllocatedType(), Save, "msga_postload", /*isVolatile=*/true,
                       AfterSetjmp),
          Var, AfterSetjmp);
      }
    }

    if (verbose)
      errs() << "Function before mem2reg: " << F << "\n";
    
    DominatorTree DT(F);
    PromoteMemToReg(AllocaVarAcrossSetjmp, DT);
  }
    
  void run() {
    for (Function &F : M)
      run(F);
  }
};

} // anonymous namespace

PreservedAnalyses MakeSetjmpsGreatAgainPass::run(Module &M,
                                                 ModuleAnalysisManager &) {
  MakeSetjmpsGreatAgain P(M);
  P.run();
  return PreservedAnalyses::none();
}
