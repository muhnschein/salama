# Writes ctest file: one test per Qt Test function of TEST_EXE, named TEST_NAME.<function>.
# Run after each link (tests/CMakeLists.txt, salama_split_test).
#
#     cmake -DTEST_EXE=<binary> -DTEST_NAME=<name> -DOUT=<file> -P split-test.cmake
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen ${TEST_EXE} -functions
    OUTPUT_VARIABLE listed
    ERROR_VARIABLE errors
    RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "${TEST_EXE} -functions failed (${status}): ${errors}")
endif()

# Qt prints `name()` per line; init/cleanup hooks not among them.
string(REGEX MATCHALL "[A-Za-z_][A-Za-z0-9_]*\\(\\)" functions "${listed}")
if(NOT functions)
    message(FATAL_ERROR "${TEST_EXE} -functions listed no test function")
endif()

set(text "")
foreach(function IN LISTS functions)
    string(REPLACE "()" "" function "${function}")
    string(APPEND text
        "add_test([==[${TEST_NAME}.${function}]==] [==[${TEST_EXE}]==] [==[${function}]==])\n"
        "set_tests_properties([==[${TEST_NAME}.${function}]==] PROPERTIES"
        " ENVIRONMENT QT_QPA_PLATFORM=offscreen)\n")
endforeach()
file(WRITE "${OUT}" "${text}")
