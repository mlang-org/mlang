add_executable(mbuild src/main.cpp)
target_link_libraries(mbuild PRIVATE mlang::frontend mlang::warnings)
install(TARGETS mbuild RUNTIME DESTINATION bin)
