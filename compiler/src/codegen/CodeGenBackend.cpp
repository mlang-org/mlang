#include "mlang/codegen/CodeGenBackend.hpp"

// The LLVM backend is compiled only when MLANG_ENABLE_LLVM is defined. Until it
// lands, this translation unit provides the factory so the rest of the compiler
// links and runs through the MIR stage on any C++23 toolchain.

namespace mlang::codegen {

#ifdef MLANG_ENABLE_LLVM
// The concrete LlvmBackend will be defined in LlvmBackend.cpp and returned here.
std::unique_ptr<CodeGenBackend> createBackend();
bool backendAvailable() { return true; }
#else
std::unique_ptr<CodeGenBackend> createBackend() { return nullptr; }
bool backendAvailable() { return false; }
#endif

} // namespace mlang::codegen
