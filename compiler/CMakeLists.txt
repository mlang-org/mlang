# The shared frontend library: lexer, AST, parser, and support utilities.
# mlangc, mfmt, and mls all build on top of this single library so the editor
# and the compiler can never disagree about the language (see docs/design/02).
add_library(mlang_frontend STATIC
    src/support/SourceManager.cpp
    src/support/Diagnostic.cpp
    src/lexer/Token.cpp
    src/lexer/Lexer.cpp
    src/ast/AstPrinter.cpp
    src/parser/Parser.cpp)
add_library(mlang::frontend ALIAS mlang_frontend)

target_include_directories(mlang_frontend PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(mlang_frontend PUBLIC mlang::warnings)

# The compiler driver.
add_executable(mlangc src/driver/main.cpp)
target_link_libraries(mlangc PRIVATE mlang::frontend mlang::warnings)
target_compile_definitions(mlangc PRIVATE MLANG_VERSION="${PROJECT_VERSION}")

install(TARGETS mlangc RUNTIME DESTINATION bin)
