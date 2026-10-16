# Wasteland Infinite additions. Native Linux is the validated build target.
add_library(wasteland-core STATIC location.cc options.cc)
target_include_directories(wasteland-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(wasteland-core PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE WASTELAND_INFINITE=1)
target_sources(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/runtime.cc)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${EXECUTABLE_NAME} wasteland-core)

if(BUILD_TESTING)
    add_executable(wasteland-tests ${PROJECT_SOURCE_DIR}/tests/location_test.cc)
    target_link_libraries(wasteland-tests wasteland-core)
    add_test(NAME wasteland-core COMMAND wasteland-tests)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_test(NAME wasteland-contract
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -s tests -p test_*.py -v)
    set_tests_properties(wasteland-contract PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        ENVIRONMENT "WI_ENGINE=$<TARGET_FILE:${EXECUTABLE_NAME}>;PYTHONDONTWRITEBYTECODE=1")
endif()
