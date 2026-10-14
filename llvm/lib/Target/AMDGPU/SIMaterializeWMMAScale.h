//===- SIMaterializeWMMAScale.h ---------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AMDGPU_SIMATERIALIZEWMMASCALE_H
#define LLVM_LIB_TARGET_AMDGPU_SIMATERIALIZEWMMASCALE_H

#include "AMDGPU.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/InitializePasses.h"

namespace llvm {

class MachineInstr;
class MachineRegisterInfo;
class SIInstrInfo;
class SIRegisterInfo;

class SIMaterializeWMMAScale {
  struct ScaleUse {
    MachineInstr *MI;
    unsigned OpNo;
  };

  MachineRegisterInfo *MRI = nullptr;
  const SIInstrInfo *TII = nullptr;
  const SIRegisterInfo *TRI = nullptr;
  const MachineLoopInfo *MLI = nullptr;

  bool materialize(Register Reg, ArrayRef<ScaleUse> Uses);

public:
  SIMaterializeWMMAScale() = default;
  bool run(MachineFunction &MF, const MachineLoopInfo &Loops);
};

class SIMaterializeWMMAScaleLegacy : public MachineFunctionPass {
public:
  static char ID;

  SIMaterializeWMMAScaleLegacy() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return "SI Materialize WMMA Scale"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.addPreserved<MachineLoopInfoWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setIsSSA();
  }
};

class SIMaterializeWMMAScalePass
    : public OptionalPassInfoMixin<SIMaterializeWMMAScalePass> {
public:
  SIMaterializeWMMAScalePass() = default;
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);

  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setIsSSA();
  }
};

#define DEBUG_TYPE "si-materialize-wmma-scale"

STATISTIC(NumWMMAScalesMaterialized,
          "Number of SGPR WMMA scales materialized into VGPRs");

/// VALU SGPR-source issue slots. depctr_va_ssrc is a 1-bit counter.
inline constexpr unsigned VaSsrcDepth = 1;

inline char SIMaterializeWMMAScaleLegacy::ID = 0;

inline char &SIMaterializeWMMAScaleLegacyID = SIMaterializeWMMAScaleLegacy::ID;

inline FunctionPass *createSIMaterializeWMMAScaleLegacyPass() {
  return new SIMaterializeWMMAScaleLegacy();
}

inline void initializeSIMaterializeWMMAScaleLegacyPass(PassRegistry &Registry) {
  static once_flag Flag;
  llvm::call_once(Flag, [&Registry] {
    initializeMachineLoopInfoWrapperPassPass(Registry);
    PassInfo *PI = new PassInfo(
        "SI Materialize WMMA Scale", DEBUG_TYPE,
        &SIMaterializeWMMAScaleLegacy::ID,
        PassInfo::NormalCtor_t(callDefaultCtor<SIMaterializeWMMAScaleLegacy>),
        false, false);
    Registry.registerPass(*PI, true);
  });
}

#undef DEBUG_TYPE

} // namespace llvm

#endif // LLVM_LIB_TARGET_AMDGPU_SIMATERIALIZEWMMASCALE_H
