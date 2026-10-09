find_package(Python3 REQUIRED COMPONENTS Interpreter)

find_program(JUCE_TIDY_CLANG_TIDY clang-tidy
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    NO_DEFAULT_PATH
    REQUIRED
)

set(JUCE_TIDY_TEST_COMPILER_ARGS
    -std=c++20
    -isystem "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(APPLE)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE JUCE_TIDY_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    list(APPEND JUCE_TIDY_TEST_COMPILER_ARGS -isysroot "${JUCE_TIDY_SDK_PATH}")
endif()

file(GLOB JUCE_TIDY_TEST_FILES CONFIGURE_DEPENDS checks/*.cpp checks/*.hpp)

foreach(test_file IN LISTS JUCE_TIDY_TEST_FILES)
    get_filename_component(test_name "${test_file}" NAME_WLE)
    string(REGEX REPLACE "\\..*$" "" check_name "${test_name}")

    add_test(
        NAME ${test_name}
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/check_clang_tidy.py"
            --clang-tidy "${JUCE_TIDY_CLANG_TIDY}"
            --plugin "$<TARGET_FILE:JuceTidy>"
            --check ${check_name}
            "${test_file}"
            --
            ${JUCE_TIDY_TEST_COMPILER_ARGS}
    )
endforeach()
