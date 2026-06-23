add_executable(mls src/main.cpp)
target_link_libraries(mls PRIVATE mlang::frontend mlang::warnings)
install(TARGETS mls RUNTIME DESTINATION bin)
