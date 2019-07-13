cmake_minimum_required(VERSION 3.4)
project(nanoprintf)

################ CppUTest

include(ExternalProject)

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/CppUTest)
set(CPPUTEST_LIB_DIR ${CMAKE_BINARY_DIR}/CppUTest/lib)
set(CPPUTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/CppUTest/include)
set(CPPUTEST_CMAKE_ARGS -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/CppUTest
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

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")

set(CMAKE_CXX_STANDARD 11)

set(nanoprintf_common_flags -pedantic -Wall -Wextra -Werror -Wno-padded)

string(FIND "${CMAKE_C_COMPILER_ID}" "Clang" is_clang)
if (${is_clang} GREATER_EQUAL 0)
    list(APPEND nanoprintf_common_flags -Weverything)
    list(APPEND nanoprintf_cxx_flags
        -Wno-old-style-cast -Wno-c++98-compat-pedantic)
endif()

set(nanoprintf_c_flags ${nanoprintf_common_flags})
set(nanoprintf_cxx_flags ${nanoprintf_common_flags} ${nanoprintf_cxx_flags})

################ Flag / language compilation tests

add_library(npf_c89 compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c89 PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1)
target_compile_options(npf_c89 PRIVATE -std=c89 ${nanoprintf_c_flags})

add_library(npf_c99 compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c99 PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1)
target_compile_options(npf_c99 PRIVATE -std=c99 ${nanoprintf_c_flags})

add_library(npf_c_no_flags compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_no_flags PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)
target_compile_options(npf_c_no_flags PRIVATE ${nanoprintf_c_flags})

add_library(npf_c_floats compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_floats PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)
target_compile_options(npf_c_floats PRIVATE -std=c99 ${nanoprintf_c_flags})

add_library(npf_c_large compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_large PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)
target_compile_options(npf_c_large PRIVATE -std=c99 ${nanoprintf_c_flags})

add_library(npf_c_writeback compilation_tests/nanoprintf_compilation_test_c.c)
target_compile_definitions(npf_c_writeback PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1)
target_compile_options(npf_c_writeback PRIVATE -std=c99 ${nanoprintf_c_flags})

add_library(npf_cpp_no_flags compilation_tests/nanoprintf_compilation_test_cpp.cpp)
target_compile_definitions(npf_cpp_no_flags PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)
target_compile_options(npf_cpp_no_flags PRIVATE ${nanoprintf_cxx_flags})

add_library(npf_cpp_floats compilation_tests/nanoprintf_compilation_test_cpp.cpp)
target_compile_definitions(npf_cpp_floats PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)
target_compile_options(npf_cpp_floats PRIVATE ${nanoprintf_cxx_flags})

add_library(npf_cpp_large compilation_tests/nanoprintf_compilation_test_cpp.cpp)
target_compile_definitions(npf_cpp_large PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)
target_compile_options(npf_cpp_large PRIVATE ${nanoprintf_cxx_flags})

add_library(npf_cpp_writeback compilation_tests/nanoprintf_compilation_test_cpp.cpp)
target_compile_definitions(npf_cpp_writeback PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1)
target_compile_options(npf_cpp_writeback PRIVATE ${nanoprintf_cxx_flags})

################ Test driver

function(npf_test name files)
    add_executable(${name} ${files})

    target_include_directories(${name} PRIVATE ${CPPUTEST_INCLUDE_DIR})
    target_link_libraries(${name} libCppUTest libCppUTestExt)

    target_compile_options(${name} PRIVATE
        ${nanoprintf_cxx_flags}
        -Wno-old-style-cast     # C style casts are ok
        -Wno-format
        -Wno-format-pedantic
        -Wno-format-nonliteral
        -std=c++11)

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

    set(timestamp "${CMAKE_CURRENT_BINARY_DIR}/${name}.timstamp")
    add_custom_target(run_${name} ALL DEPENDS ${timestamp})

    add_custom_command(OUTPUT ${timestamp}
        COMMAND ${name} && ${CMAKE_COMMAND} -E touch ${timestamp}
        DEPENDS ${name}
        COMMENT "Running ${name}")
endfunction()

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

npf_test(unit_tests "${unit_test_files}")

################ Conformance tests

set(conformance_test_files
    nanoprintf.h
    conformance_tests/nanoprintf_in_conformance_tests.cpp
    conformance_tests/test_conformance.cpp
    conformance_tests/main.cpp)

npf_test(conformance_no_flags "${conformance_test_files}")
target_compile_definitions(conformance_no_flags PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)

npf_test(conformance_float "${conformance_test_files}")
target_compile_definitions(conformance_float PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)

npf_test(conformance_large "${conformance_test_files}")
target_compile_definitions(conformance_large PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0)

npf_test(conformance_writeback "${conformance_test_files}")
target_compile_definitions(conformance_writeback PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1)

npf_test(conformance_all "${conformance_test_files}")
target_compile_definitions(conformance_all PRIVATE
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1
    NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1)
