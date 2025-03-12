set(TEST_SOURCE_FILES
    # ${CMAKE_CURRENT_SOURCE_DIR}/ww_memory_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ww_stack_test.cpp
)

add_executable(test ${TEST_SOURCE_FILES})

target_link_libraries(test PRIVATE
    containers
    GTest::gtest
    GTest::gtest_main
)