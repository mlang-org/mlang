cmake_minimum_required(VERSION 3.28)

project(MLang
    VERSION 0.1.0
    DESCRIPTION "The MLang programming language toolchain"
    HOMEPAGE_URL "https://github.com/mlang-org/mlang"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(MLANG_ENABLE_LLVM   "Build the LLVM-based native backend" OFF)
option(MLANG_BUILD_TOOLS   "Build mfmt, mls and mbuild"          ON)
option(MLANG_BUILD_RUNTIME "Build the mlang-runtime library"     ON)
option(MLANG_BUILD_TESTS   "Build the test suite"                ON)
option(MLANG_BUNDLE_MANGO  "Bundle the mango package manager (packages/mango)" ON)
option(MLANG_WARNINGS_AS_ERRORS "Treat warnings as errors"       OFF)

include(GNUInstallDirs)
include(CompilerWarnings)

# ---------------------------------------------------------------------------
# Components
# ---------------------------------------------------------------------------
add_subdirectory(compiler)

if(MLANG_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

if(MLANG_BUILD_TOOLS)
    add_subdirectory(tools/mfmt)
    add_subdirectory(tools/mls)
    add_subdirectory(tools/mbuild)
endif()

# Bundle the mango package manager as an optional installer component. mango is
# a submodule (packages/mango); it is only added when checked out.
if(MLANG_BUNDLE_MANGO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packages/mango/CMakeLists.txt")
    set(MANGO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(packages/mango)
    set(MLANG_HAVE_MANGO ON)
endif()

if(MLANG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(compiler/tests)
endif()

# Install the standard library and documentation as part of the core component.
install(DIRECTORY stdlib/std
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mlang/stdlib
    COMPONENT core)
install(FILES README.md LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT core)

include(Packaging)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "MLang ${PROJECT_VERSION} configuration")
message(STATUS "  C++ compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "  LLVM backend        : ${MLANG_ENABLE_LLVM}")
message(STATUS "  Build tools         : ${MLANG_BUILD_TOOLS}")
message(STATUS "  Build runtime       : ${MLANG_BUILD_RUNTIME}")
message(STATUS "  Build tests         : ${MLANG_BUILD_TESTS}")
message(STATUS "")
