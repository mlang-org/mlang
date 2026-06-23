#ifndef MLANG_CODEGEN_CODEGENBACKEND_HPP
#define MLANG_CODEGEN_CODEGENBACKEND_HPP

#include "mlang/ir/Mir.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace mlang::codegen {

// Target description, kept separate from the host so the compiler is a
// cross-compiler from day one (docs/design/02-architecture.md, 29-abi.md).
struct TargetInfo {
    std::string triple = "x86_64-unknown-linux-gnu";
    unsigned pointerSize = 8;
    unsigned pointerAlign = 8;
    bool littleEndian = true;
};

// The abstract code-generation interface. The rest of the compiler depends only
// on this; a concrete backend (LlvmBackend) is registered when available. When
// no backend is compiled in, createBackend() returns nullptr and the driver
// stops after MIR (docs/design/11-llvm-codegen.md).
class CodeGenBackend {
public:
    virtual ~CodeGenBackend() = default;

    virtual std::string_view name() const = 0;
    virtual void lowerModule(const ir::Module& module, const TargetInfo& target) = 0;
    virtual bool emitObject(std::string_view path) = 0;
    virtual bool emitAssembly(std::string_view path) = 0;
};

// Returns the configured backend, or nullptr if none is available in this build.
std::unique_ptr<CodeGenBackend> createBackend();

// True if this build was compiled with a native backend (MLANG_ENABLE_LLVM).
bool backendAvailable();

} // namespace mlang::codegen

#endif
