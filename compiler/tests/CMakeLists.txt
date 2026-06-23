add_executable(frontend_tests test_main.cpp)
target_link_libraries(frontend_tests PRIVATE mlang::frontend mlang::warnings)

add_test(NAME frontend_tests COMMAND frontend_tests)
