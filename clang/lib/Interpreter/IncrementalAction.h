//===--- IncrementalAction.h - Incremental Frontend Action -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_INTERPRETER_INCREMENTALACTION_H
#define LLVM_CLANG_INTERPRETER_INCREMENTALACTION_H

#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/MultiplexConsumer.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {
class Module;
}

namespace clang {

class Decl;
class Interpreter;
class CodeGenerator;

/// A custom action enabling the incremental processing functionality.
///
/// The usual \p FrontendAction expects one call to ExecuteAction and once it
/// sees a call to \p EndSourceFile it deletes some of the important objects
/// such as \p Preprocessor and \p Sema assuming no further input will come.
///
/// \p IncrementalAction ensures it keep its underlying action's objects alive
/// as long as the \p IncrementalParser needs them.
///
class IncrementalAction : public WrapperFrontendAction {
private:
  bool IsTerminating = false;
  Interpreter &Interp;
  [[maybe_unused]] CompilerInstance &CI;
  std::unique_ptr<ASTConsumer> Consumer;

  /// When CodeGen is created the first llvm::Module gets cached in many places
  /// and we must keep it alive.
  std::unique_ptr<llvm::Module> CachedInCodeGenModule;

  /// Definitions CodeGen refused while the diagnostics were in an error state,
  /// waiting to be handed over once they are reset.
  llvm::SmallVector<Decl *, 4> DeclsToReemit;

public:
  IncrementalAction(CompilerInstance &Instance, llvm::LLVMContext &LLVMCtx,
                    llvm::Error &Err, Interpreter &I,
                    std::unique_ptr<ASTConsumer> Consumer = nullptr);

  FrontendAction *getWrapped() const { return WrappedAction.get(); }

  TranslationUnitKind getTranslationUnitKind() override {
    return TU_Incremental;
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;

  void ExecuteAction() override;

  // Do not terminate after processing the input. This allows us to keep various
  // clang objects alive and to incrementally grow the current TU.
  void EndSourceFile() override;

  void FinalizeAction();

  /// Cache the current CodeGen module to preserve internal references.
  void CacheCodeGenModule();

  /// Access the cached CodeGen module.
  llvm::Module *getCachedCodeGenModule() const;

  /// Access the current code generator.
  CodeGenerator *getCodeGen() const;

  /// Generate an LLVM module for the most recent parsed input.
  std::unique_ptr<llvm::Module> GenModule();

  /// Throw away what CodeGen emitted for an input that failed to parse and
  /// start a fresh module.
  void discardModule();

  /// Remember a definition CodeGen would refuse right now, because an input
  /// raised an error and CodeGen emits nothing while that is the case.
  void deferReemission(Decl *D) { DeclsToReemit.push_back(D); }

  /// Hand over the definitions collected by deferReemission(). Only meaningful
  /// once the diagnostics have been reset, since that is what made CodeGen
  /// refuse them in the first place.
  void reemitDeferredDecls();
};

class InProcessPrintingASTConsumer final : public MultiplexConsumer {
  Interpreter &Interp;
  IncrementalAction &Act;

public:
  InProcessPrintingASTConsumer(std::unique_ptr<ASTConsumer> C, Interpreter &I,
                               IncrementalAction &Act);

  bool HandleTopLevelDecl(DeclGroupRef DGR) override;
};

} // end namespace clang

#endif // LLVM_CLANG_INTERPRETER_INCREMENTALACTION_H
