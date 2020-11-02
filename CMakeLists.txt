cmake_minimum_required(VERSION 3.4)
project(nanoprintf)

option(NPF_32BIT "Compile nanoprintf tests in 32-bit mode")

if (NPF_32BIT AND CMAKE_HOST_APPLE)
    message(FATAL_ERROR "Apple doesn't support 32-bit mode anymore.")
endif()

if (NPF_32BIT AND NOT MSVC)
    set(NPF_32BIT_FLAG -m32)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${NPF_32BIT_FLAG})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${NPF_32BIT_FLAG})
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-poison-system-directories")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-poison-system-directories")
endif()

################ CppUTest

include(ExternalProject)

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/CppUTest)
set(CPPUTEST_LIB_DIR ${CMAKE_BINARY_DIR}/CppUTest/lib)
set(CPPUTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/CppUTest/include)
set(CPPUTEST_CMAKE_ARGS -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/CppUTest
                        -DCPPUTEST_C_FLAGS=${NPF_32BIT_FLAG}
                        -DCPPUTEST_CXX_FLAGS=${NPF_32BIT_FLAG}
                        -DC++11=ON
                        -DTESTS=OFF)
set(CPPUTEST_LIB_NAME
    ${CMAKE_STATIC_LIBRARY_PREFIX}CppUTest${CMAKE_STATIC_LIBRARY_SUFFIX})

set(CPPUTEST_LIB "${CPPUTEST_LIB_DIR}/${CPPUTEST_LIB_NAME}")

set(CPPUTESTEXT_LIB_NAME
    ${CMAKE_STATIC_LIBRARY_PREFIX}CppUTestExt${CMAKE_STATIC_LIBRARY_SUFFIX})

set(CPPUTESTEXT_LIB "${CPPUTEST_LIB_DIR}/${CPPUTESTEXT_LIB_NAME}")

ExternalProject_Add(CppUTest_external
                    PREFIX ${CPPUTEST_ROOT_DIR}
                    GIT_REPOSITORY "https://github.com/cpputest/cpputest.git"
                    GIT_TAG "8baa7645cc7fe4c18cdaf0c91eccec72438f6ce8"
                    UPDATE_COMMAND ""
                    PATCH_COMMAND ""
                    TEST_COMMAND ""
                    CMAKE_ARGS ${CPPUTEST_CMAKE_ARGS}
                    BINARY_DIR ${CMAKE_BINARY_DIR}/CppUTest
                    INSTALL_DIR ${CMAKE_BINARY_DIR}/CppUTest
                    BUILD_BYPRODUCTS "${CPPUTEST_LIB}" "${CPPUTESTEXT_LIB}")

add_library(libCppUTest STATIC IMPORTED)
set_target_properties(libCppUTest PROPERTIES IMPORTED_LOCATION "${CPPUTEST_LIB}")

add_library(libCppUTestExt STATIC IMPORTED)
set_target_properties(libCppUTestExt PROPERTIES IMPORTED_LOCATION "${CPPUTESTEXT_LIB}")

add_dependencies(libCppUTest CppUTest_external)
add_dependencies(libCppUTestExt CppUTest_external)

################ Common compile flags

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    set(nanoprintf_common_flags /W4)
else()
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")
    set(CMAKE_C_FLAGS_RELEASE "-Os")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
    set(nanoprintf_common_flags
        -pedantic
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wsign-conversion
        -Wswitch-default
        -Wswitch-enum
        -Wno-padded
        -Wno-covered-switch-default)
endif()

string(FIND "${CMAKE_C_COMPILER_ID}" "Clang" is_clang)
if (${is_clang} GREATER_EQUAL 0)
    list(APPEND nanoprintf_common_flags -Weverything)
    list(APPEND nanoprintf_cxx_flags
        -Wno-old-style-cast -Wno-c++98-compat-pedantic)
endif()

set(nanoprintf_c_flags ${nanoprintf_common_flags})
set(nanoprintf_cxx_flags ${nanoprintf_common_flags} ${nanoprintf_cxx_flags})

################ CppUtest test driver

function(npf_test name files)
    add_executable(${name} ${files})

    target_include_directories(${name} PRIVATE ${CPPUTEST_INCLUDE_DIR})
    target_link_libraries(${name} libCppUTest libCppUTestExt)

    if (MSVC)
        target_link_libraries(${name} winmm.lib)
    else()
        target_compile_options(${name} PRIVATE
            ${nanoprintf_cxx_flags}
            -Wno-old-style-cast     # C style casts are ok
            -Wno-format
            -Wno-format-pedantic
            -Wno-format-nonliteral)
    endif()

    if (is_clang GREATER_EQUAL 0)
        target_compile_options(${name} PRIVATE
            -Wno-c++11-long-long    # CppUTest uses long long
            -Wno-reserved-id-macro  # CppUTest uses leading __
            -Wno-keyword-macro      # CppUTest redefines 'new' for memory checks
            -Wno-disabled-macro-expansion
            -Wno-weak-vtables
            -Wno-global-constructors   # CppUTest memory checks
            -Wno-exit-time-destructors # CppUTest memory checks
            -Wno-c++98-compat
            -Wno-missing-prototypes)
    endif()

    set(timestamp "${CMAKE_CURRENT_BINARY_DIR}/${name}.timestamp")
    add_custom_target(run_${name} ALL DEPENDS ${timestamp})

    add_custom_command(OUTPUT ${timestamp}
        COMMAND ${name} && ${CMAKE_COMMAND} -E touch ${timestamp}
        DEPENDS ${name}
        COMMENT "Running ${name}")
endfunction()

################ Conformance test sources

set(conformance_test_files
    nanoprintf.h
    conformance_tests/nanoprintf_in_conformance_tests.cpp
    conformance_tests/test_conformance.cpp
    conformance_tests/main.cpp)

################ Language compilation tests

function(npf_c_language_test target)
    add_library(${target} compilation_tests/nanoprintf_compilation_test_c.c)
    target_compile_options(${target} PRIVATE ${nanoprintf_c_flags})
endfunction()

# Test every combination of compatible flags.
foreach(fw 0 1)
    foreach(precision 0 1)
        foreach(large 0 1)
            foreach(float 0 1)
                foreach(wb 0 1)
                    if ((precision EQUAL 0) AND (float EQUAL 1))
                        continue()
                    endif()
                    set(standard c99)
                    set(test_name "${standard}")
                    if (fw EQUAL 1)
                        string(APPEND test_name "_fieldwidth")
                    endif()
                    if (precision EQUAL 1)
                        string(APPEND test_name "_precision")
                    endif()
                    if (large EQUAL 1)
                        string(APPEND test_name "_large")
                    endif()
                    if (float EQUAL 1)
                        string(APPEND test_name "_float")
                    endif()
                    if (wb EQUAL 1)
                        string(APPEND test_name "_writeback")
                    endif()

                    # Run a simple compilation test
                    set(compilation_test_name "npf_compile_${test_name}")
                    npf_c_language_test(${compilation_test_name})
                    target_compile_definitions(${compilation_test_name} PRIVATE
                        NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=${fw}
                        NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=${precision}
                        NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=${large}
                        NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=${float}
                        NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=${wb})

                    # Run conformance tests (c++)
                    set(conformance_test_name "npf_conform_${test_name}")
                    npf_test(${conformance_test_name} "${conformance_test_files}")
                    target_compile_definitions(${conformance_test_name} PRIVATE
                        NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=${fw}
                        NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=${precision}
                        NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=${large}
                        NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=${float}
                        NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=${wb})
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endforeach()

# Test default flags (nothing specified)
npf_c_language_test(npf_c_default_flags)

################ Static compilation test

add_executable(npf_static
    static_test/nanoprintf_in_static_test.c
    static_test/main.c)

################ Unit tests

set(unit_test_files
    nanoprintf.h
    unit_tests/test_bufputc.cpp
    unit_tests/test_parse_format_spec.cpp
    unit_tests/test_ftoa_rev.cpp
    unit_tests/test_fsplit_abs.cpp
    unit_tests/test_itoa_rev.cpp
    unit_tests/test_ptoa_rev.cpp
    unit_tests/test_utoa_rev.cpp
    unit_tests/test_snprintf.cpp
    unit_tests/test_vpprintf.cpp
    unit_tests/nanoprintf_in_unit_tests.cpp
    unit_tests/nanoprintf_in_unit_tests.h
    unit_tests/main.cpp)

npf_test(unit_tests_normal_sized_formatters "${unit_test_files}")
target_compile_definitions(unit_tests_normal_sized_formatters PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0)

npf_test(unit_tests_large_sized_formatters "${unit_test_files}")
target_compile_definitions(unit_tests_large_sized_formatters PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1)

################ Examples

add_executable(use_npf_directly
    examples/use_npf_directly/your_project_nanoprintf.h
    examples/use_npf_directly/your_project_nanoprintf.cpp
    examples/use_npf_directly/main.cpp)

add_executable(wrap_npf
    examples/wrap_npf/your_project_printf.h
    examples/wrap_npf/your_project_printf.cpp
    examples/wrap_npf/main.cpp)

