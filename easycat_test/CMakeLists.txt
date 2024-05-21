
set(SOURCES easycat_test.c)
add_executable(easycat_test ${SOURCES})
target_link_libraries(easycat_test soem)
install(TARGETS easycat_test DESTINATION bin)
