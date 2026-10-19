cmake_minimum_required(VERSION 3.25)
project(JuceTidy
    VERSION 0.5.0
    DESCRIPTION "JUCE-specific checks for clang-tidy"
    LANGUAGES C CXX
)

option(JUCE_TIDY_BUILD_TESTS "Build and register the check tests?" ON)

find_package(Clang 23.1 REQUIRED CONFIG)
message(STATUS "Building against LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_INSTALL_PREFIX}")

add_library(JuceTidy MODULE
    source/JuceTidyModule.cpp
    source/utils/JuceTidyUtils.cpp
)

target_compile_features(JuceTidy PRIVATE cxx_std_23)
target_include_directories(JuceTidy
    PRIVATE
        source
)
target_include_directories(JuceTidy
    SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS}
)

separate_arguments(JUCE_TIDY_LLVM_DEFINITIONS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(JuceTidy PRIVATE ${JUCE_TIDY_LLVM_DEFINITIONS})

if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(JuceTidy PRIVATE -fno-rtti)
endif()

# The clang-tidy executable provides every clang and clang-tidy symbol at load time
if(APPLE)
    target_link_options(JuceTidy PRIVATE -undefined dynamic_lookup)
endif()

set_target_properties(JuceTidy
    PROPERTIES
        PREFIX ""
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)

include(GNUInstallDirs)
install(TARGETS JuceTidy LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(JUCE_TIDY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
