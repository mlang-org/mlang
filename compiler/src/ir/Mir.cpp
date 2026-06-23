#include "mlang/ir/Mir.hpp"

#include <ostream>

namespace mlang::ir {

Block& Function::addBlock(std::string label) {
    Block b;
    b.index = static_cast<std::uint32_t>(blocks.size());
    b.label = std::move(label);
    blocks.push_back(std::move(b));
    return blocks.back();
}

Function& Module::addFunction(std::string fnName) {
    auto fn = std::make_unique<Function>();
    fn->name = std::move(fnName);
    functions.push_back(std::move(fn));
    return *functions.back();
}

namespace {
const char* termName(TermKind k) {
    switch (k) {
    case TermKind::Br: return "br";
    case TermKind::CondBr: return "cond_br";
    case TermKind::Switch: return "switch";
    case TermKind::Ret: return "ret";
    case TermKind::Unwind: return "unwind";
    case TermKind::Unreachable: return "unreachable";
    }
    return "?";
}
} // namespace

void Module::print(std::ostream& os) const {
    os << "; MIR module: " << name << '\n';
    for (const auto& fn : functions) {
        os << "\nfn @" << fn->name << "(";
        for (std::size_t i = 0; i < fn->paramTypes.size(); ++i) {
            if (i) os << ", ";
            os << (fn->paramTypes[i] ? fn->paramTypes[i]->toString() : "?");
        }
        os << ") -> " << (fn->returnType ? fn->returnType->toString() : "void") << " {\n";
        for (const auto& block : fn->blocks) {
            os << "  " << block.label << ":\n";
            for (const auto& inst : block.instructions) {
                os << "    ";
                if (inst.result != kNoValue) {
                    os << "%" << inst.result << " = ";
                }
                os << static_cast<int>(inst.op);
                for (ValueId op : inst.operands) {
                    os << " %" << op;
                }
                os << '\n';
            }
            os << "    " << termName(block.terminator.kind) << '\n';
        }
        os << "}\n";
    }
}

} // namespace mlang::ir
