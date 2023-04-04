project(BN3MONKEY_TEST)

set(LIBRARY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../library")
message("LIBRARY_SOURCE_DIR is ${LIBRARY_SOURCE_DIR}")

set(FRAMEWORK_SOURCE_DIR "${LIBRARY_SOURCE_DIR}/framework")
message("FRAMEWORK_SOURCE_DIR is ${FRAMEWORK_SOURCE_DIR}")

set(TEST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(FRAMEWORK_TEST_SOURCE_DIR "${FRAMEWORK_TEST_SOURCE_DIR}/framework")

file(GLOB_RECURSE TEST_SOURCE_FILES
    ${TEST_SOURCE_DIR}/*.h
    ${TEST_SOURCE_DIR}/*.hpp
    ${TEST_SOURCE_DIR}/*.cpp
    ${TEST_SOURCE_DIR}/*.c)
message("TEST_SOURCE_FILES is ${TEST_SOURCE_FILES}")

add_executable(
    bn3monkey_test
    ${TEST_SOURCE_FILES}
 "framework/test_helper.cpp")

target_include_directories(
    bn3monkey_test
    PRIVATE
    ${FRAMEWORK_SOURCE_DIR}
)

add_dependencies(bn3monkey_test bn3monkey_library)

target_link_libraries(bn3monkey_test
    PUBLIC
    bn3monkey_library)



set_property(TARGET bn3monkey_test PROPERTY CXX_STANDARD 17)
set_property(TARGET bn3monkey_test PROPERTY CXX_STANDARD_REQUIRED ON)