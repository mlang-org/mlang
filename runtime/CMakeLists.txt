# mlang-runtime: the small native library every MLang program links against
# (allocator, ARC, panics, startup). See docs/design/12-runtime.md.
add_library(mlang_runtime STATIC
    src/Runtime.cpp)
add_library(mlang::runtime ALIAS mlang_runtime)

target_include_directories(mlang_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(mlang_runtime PRIVATE mlang::warnings)

install(TARGETS mlang_runtime ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT core)
