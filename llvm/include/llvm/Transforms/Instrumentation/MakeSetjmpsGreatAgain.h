#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_MAKESETJMPSGREATAGAIN_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_MAKESETJMPSGREATAGAIN_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Module;

class MakeSetjmpsGreatAgainPass
    : public PassInfoMixin<MakeSetjmpsGreatAgainPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif /* LLVM_TRANSFORMS_INSTRUMENTATION_MAKESETJMPSGREATAGAIN_H */

