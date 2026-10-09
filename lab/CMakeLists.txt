# IOSretroMob public lab: the same emucore rasterizer, no firmware fixture.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(iosretromob-replay replay.c)
target_link_libraries(iosretromob-replay PRIVATE emucore)
add_test(NAME iosretromob_public_replays COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_lab.py" "${CMAKE_BINARY_DIR}")
add_test(NAME iosretromob_diagnostic_tools COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_SOURCE_DIR}/tests/guest-apps" -p "test_*.py")
set_tests_properties(iosretromob_public_replays iosretromob_diagnostic_tools PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")

add_test(NAME iosretromob_gallery COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/iosretromob" verify-gallery)
set_tests_properties(iosretromob_gallery PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
