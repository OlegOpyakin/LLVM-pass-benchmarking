#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include <set>
#include <tuple>
using namespace llvm;

namespace {
  struct ElementInfo {
    int coeff_;
    int power_A_;
    int power_B_;
  };

  struct SkeletonPass : public PassInfoMixin<SkeletonPass> {

    Value* lookThroughLoad(Value *V) {
      if (LoadInst *LI = dyn_cast<LoadInst>(V)) {
        if (AllocaInst *AI = dyn_cast<AllocaInst>(LI->getOperand(0))) {
          for (User *U : AI->users()) {
            if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
              if (SI->getOperand(1) == AI) {
                Value *StoredValue = SI->getOperand(0);
                if (isa<Argument>(StoredValue)) {
                  return StoredValue;
                }
              }
            }
          }
        }
      }
      return V;
    }

    // ADD instructions extractor: (a + (b + c)) -> [a, b, c]
    bool ExtractADD(Value *V, SmallVector<Value*, 4> &AddVector){
      if(BinaryOperator *op = dyn_cast<BinaryOperator>(V)){
        if(op->getOpcode() == Instruction::Add){
          return ExtractADD(op->getOperand(0), AddVector) &&
                 ExtractADD(op->getOperand(1), AddVector);
        }
        AddVector.push_back(V);
        return true;
      }
      AddVector.push_back(V);
      return true;
    }

    // MUL decomposer: 3 * a * b -> [3, a, b]
    bool ExtractMUL(Value *V, SmallVector<Value*, 8> &MulVector){
      if(BinaryOperator *op = dyn_cast<BinaryOperator>(V)){
        if(op->getOpcode() == Instruction::Mul){
          return ExtractMUL(op->getOperand(0), MulVector) &&
                 ExtractMUL(op->getOperand(1), MulVector);
        }
      }
      MulVector.push_back(V);
      return true;
    }
    
    // Function to fill the structure
    bool FillElementInfo(Value *Term, Value *A, Value *B, ElementInfo &elem){
      SmallVector<Value*, 8> info_vec;
      if (!ExtractMUL(Term, info_vec)) return false;

      elem.coeff_ = 1;
      elem.power_A_ = 0;
      elem.power_B_ = 0;

      for(auto &V: info_vec){
        Value *ActualV = lookThroughLoad(V);
        
        if(ConstantInt *c = dyn_cast<ConstantInt>(ActualV)){
          elem.coeff_*= c->getSExtValue();
        }
        else if(ActualV == A){
          ++elem.power_A_;
        }
        else if(ActualV == B){
          ++elem.power_B_;
        }
        else{
          if(ConstantInt *c = dyn_cast<ConstantInt>(V)){
            elem.coeff_*= c->getSExtValue();
          }
          else if(V == A){
            ++elem.power_A_;
          }
          else if(V == B){
            ++elem.power_B_;
          }
          else{
            return false;
          }
        }
      }
      return true;
    }

    // Main function
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
      bool modified = false;
      
      for(auto &B: F){
        for(auto &I: B){
          if(I.getOpcode() == Instruction::Add){
            SmallVector<Value*, 4> elems_of_sum;
            if(!ExtractADD(&I, elems_of_sum) || elems_of_sum.size() != 4) continue;

            SmallVector<Value*, 8> non_const_elems;
            for(auto &Term: elems_of_sum){
              SmallVector<Value*, 8> elems_of_multiplication;
              if(!ExtractMUL(Term, elems_of_multiplication)) continue;
              for(Value *V: elems_of_multiplication){
                if(!isa<Constant>(V)) {
                  Value *ActualV = lookThroughLoad(V);
                  if(!isa<Constant>(ActualV)) {
                    non_const_elems.push_back(ActualV);
                  } else if(!isa<Constant>(V)) {
                    non_const_elems.push_back(V);
                  }
                }
              }
            }
            
            std::sort(non_const_elems.begin(), non_const_elems.end());
            auto last = std::unique(non_const_elems.begin(), non_const_elems.end());
            non_const_elems.erase(last, non_const_elems.end());

            if(non_const_elems.size() != 2) continue;

            Value *A = non_const_elems[0];
            Value *B = non_const_elems[1];

            SmallVector<ElementInfo, 4> elems_information;
            for(auto &Term: elems_of_sum){
              ElementInfo info;
              if(!FillElementInfo(Term, A, B, info)) break;
              elems_information.push_back(info);
            }

            if(elems_information.size() != 4) continue;

            std::multiset<std::tuple<int, int, int>> current;
            for(const ElementInfo &elt: elems_information){
              current.insert({elt.coeff_, elt.power_A_, elt.power_B_});
            }

            std::multiset<std::tuple<int, int, int>> expected = {
              {1, 3, 0}, // a³
              {3, 2, 1}, // 3a²b
              {3, 1, 2}, // 3ab²
              {1, 0, 3}  // b³
            };

            if(current != expected) continue;

            IRBuilder<> Builder(&I);
            Value *AddAB = Builder.CreateAdd(A, B, "cubic_a_plus_b");
            Value *ABSquared = Builder.CreateMul(AddAB, AddAB, "cubic_squared");
            Value *ABCube = Builder.CreateMul(ABSquared, AddAB, "cubic_result");

            I.replaceAllUsesWith(ABCube);
            modified = true;
            
            break;
					} 
        }
        if(modified) break;
      }

      return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
  };
}

// Register the pass with the new pass manager
llvm::PassPluginLibraryInfo getSkeletonPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "skeleton-pass", "v1.0",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
          [](StringRef Name, FunctionPassManager &FPM,
          ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "skeleton-pass") {
            FPM.addPass(SkeletonPass());
            return true;
          }
          return false;
        });
      
      PB.registerPipelineStartEPCallback(
          [](ModulePassManager &MPM, OptimizationLevel OL) {
            FunctionPassManager FPM;
            FPM.addPass(SkeletonPass());
            MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
          });
    }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getSkeletonPassPluginInfo();
}
