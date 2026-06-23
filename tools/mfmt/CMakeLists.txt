add_executable(mfmt src/main.cpp)
target_link_libraries(mfmt PRIVATE mlang::frontend mlang::warnings)
install(TARGETS mfmt RUNTIME DESTINATION bin)
