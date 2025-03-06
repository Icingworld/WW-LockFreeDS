if(ENABLE_TEST)
    add_subdirectory(gtest)
endif()

if(ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif()
