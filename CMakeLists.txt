cmake_minimum_required(VERSION 3.4)
project(nanoprintf C CXX)

################ CppUTest

include(ExternalProject)

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/CppUTest)
set(CPPUTEST_LIB_DIR ${CMAKE_BINARY_DIR}/CppUTest/lib)
set(CPPUTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/CppUTest/include)
set(CPPUTEST_CMAKE_ARGS -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/CppUTest
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

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")
set(CMAKE_CXX_STANDARD 11)

string(CONCAT nanoprintf_common_flags
    "-pedantic "
    "-Wall "
    "-Weverything "
    "-Werror "
    "-Wno-old-style-cast "
    "-Wno-c++98-compat-pedantic "
    "-Wno-padded")

################ Flag / language compilation tests

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${nanoprintf_common_flags}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${nanoprintf_common_flags}")

add_library(npf_c89 compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c89 PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0)
target_compile_options(npf_c89 PRIVATE -std=c89)

add_library(npf_c99 compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c99 PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1)
target_compile_options(npf_c99 PRIVATE -std=c99)

add_library(npf_c11 compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c11 PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1)
target_compile_options(npf_c11 PRIVATE -std=c11)

add_library(npf_c_no_flags compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_no_flags PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0)

add_library(npf_c_floats compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_floats PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1)
target_compile_options(npf_c_floats PRIVATE -std=c99)

add_library(npf_c_c99 compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_c99 PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0)
target_compile_options(npf_c_c99 PRIVATE -std=c99)

add_library(npf_cc_no_flags compilation_tests/nanoprintf_compilation_test_cc.cc)
target_compile_definitions(npf_cc_no_flags PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0)

add_library(npf_cc_floats compilation_tests/nanoprintf_compilation_test_cc.cc)
target_compile_definitions(npf_cc_floats PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1)

add_library(npf_cc_c99 compilation_tests/nanoprintf_compilation_test_cc.cc)
target_compile_definitions(npf_cc_c99 PRIVATE
    NANOPRINTF_USE_C99_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0)

################ Unit tests

add_executable(npf_unit_tests
    nanoprintf.h
    unit_tests/test_bufputc.cc
    unit_tests/test_conformance.cc
    unit_tests/test_parse_format_spec.cc
    unit_tests/test_ftoa_rev.cc
    unit_tests/test_fsplit_abs.cc
    unit_tests/test_itoa_rev.cc
    unit_tests/test_ptoa_rev.cc
    unit_tests/test_utoa_rev.cc
    unit_tests/test_vpprintf.cc
    unit_tests/nanoprintf_in_unit_tests.cc
    unit_tests/nanoprintf_in_unit_tests.h
    unit_tests/main.cc)

set_source_files_properties(unit_tests/test_conformance.cc
    PROPERTIES COMPILE_FLAGS -Wno-format-nonliteral)

target_include_directories(npf_unit_tests PRIVATE "${CPPUTEST_INCLUDE_DIR}")
target_link_libraries(npf_unit_tests libCppUTest libCppUTestExt)

target_compile_options(npf_unit_tests PRIVATE
    -Wno-c++11-long-long    # CppUTest uses long long
    -Wno-reserved-id-macro  # CppUTest uses leading __
    -Wno-old-style-cast     # C style casts are ok
    -Wno-keyword-macro      # CppUTest redefines 'new' for memory checks
    -Wno-disabled-macro-expansion
    -Wno-weak-vtables
    -Wno-global-constructors   # CppUTest memory checks
    -Wno-exit-time-destructors # CppUTest memory checks
    -Wno-c++98-compat
    -Wno-missing-prototypes
    -std=c++11
)

set(timestamp ${CMAKE_CURRENT_BINARY_DIR}/npf_unit_tests.timestamp)
add_custom_target(run_npf_unit_tests ALL DEPENDS ${timestamp})

add_custom_command(OUTPUT ${timestamp}
    COMMAND npf_unit_tests && ${CMAKE_COMMAND} -E touch ${timestamp}
    DEPENDS npf_unit_tests
    COMMENT "Running unit tests")
