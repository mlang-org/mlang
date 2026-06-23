#ifndef MLANG_IR_MIR_HPP
#define MLANG_IR_MIR_HPP

#include "mlang/types/Type.hpp"

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace mlang::ir {

// MLang mid-level IR: a typed, SSA, control-flow-graph representation. This
// header defines the data model described in docs/design/09-ir.md. Lowering
// (MirBuilder) and the optimizer passes are built incrementally on top.

enum class Opcode {
    // pure
    ConstInt, ConstFloat, ConstStr, ConstBool, ConstNull,
    Add, Sub, Mul, Div, Rem,
    CmpEq, CmpNe, CmpLt, CmpLe, CmpGt, CmpGe,
    And, Or, Not, Neg,
    Field, Index, Cast,
    // memory
    Alloca, Load, Store,
    // ownership (explicit in MIR so the optimizer can elide them)
    Retain, Release, Drop, Move, Borrow, EndBorrow,
    // calls
    Call, CallVirtual, CallAsync, CallIntrinsic,
    // phi
    Phi,
};

enum class TermKind { Br, CondBr, Switch, Ret, Unreachable, Unwind };

using ValueId = std::uint32_t;
inline constexpr ValueId kNoValue = 0xFFFFFFFFu;

struct Instruction {
    Opcode op;
    ValueId result = kNoValue;
    const types::Type* type = nullptr;
    std::vector<ValueId> operands;
    std::string symbol;          // call target / field name
    std::int64_t immInt = 0;     // const int / switch tag
    std::string immStr;          // const string
};

struct Terminator {
    TermKind kind = TermKind::Unreachable;
    ValueId condition = kNoValue;
    std::vector<std::uint32_t> targets; // successor block indices
    ValueId returnValue = kNoValue;
};

struct Block {
    std::uint32_t index = 0;
    std::string label;
    std::vector<Instruction> instructions;
    Terminator terminator;
};

struct Function {
    std::string name;
    std::vector<const types::Type*> paramTypes;
    const types::Type* returnType = nullptr;
    bool isAsync = false;
    std::vector<Block> blocks;
    ValueId nextValue = 0;

    Block& addBlock(std::string label);
    ValueId freshValue() { return nextValue++; }
};

struct Module {
    std::string name;
    std::vector<std::unique_ptr<Function>> functions;

    Function& addFunction(std::string name);
    void print(std::ostream& os) const;
};

} // namespace mlang::ir

#endif
