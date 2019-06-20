cmake_minimum_required(VERSION 3.4)
project(nanoprintf C CXX)

############### Global flags

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

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
set(CPPUTEST_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}CppUTest${CMAKE_STATIC_LIBRARY_SUFFIX})
set(CPPUTESTEXT_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}CppUTestExt${CMAKE_STATIC_LIBRARY_SUFFIX})
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
                    BUILD_BYPRODUCTS ${CPPUTEST_LIB_DIR}/${CPPUTEST_LIB_NAME} ${CPPUTEST_LIB_DIR}/${CPPUTESTEXT_LIB_NAME})

add_library(libCppUTest STATIC IMPORTED)
set_target_properties(libCppUTest PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/${CPPUTEST_LIB_NAME})

add_library(libCppUTestExt STATIC IMPORTED)
set_target_properties(libCppUTestExt PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/${CPPUTESTEXT_LIB_NAME})

add_dependencies(libCppUTest CppUTest_external)
add_dependencies(libCppUTestExt CppUTest_external)

################ Test support

function(npf_add_test TEST_TARGET_EXECUTABLE)
    set(TEST_TIMESTAMP_FILE
        ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET_EXECUTABLE}_tests.timestamp)
    add_custom_target(RUN_${TEST_TARGET_EXECUTABLE}_TESTS
        ALL DEPENDS ${TEST_TIMESTAMP_FILE})
    add_custom_command(OUTPUT ${TEST_TIMESTAMP_FILE}
        COMMAND ${TEST_TARGET_EXECUTABLE} && ${CMAKE_COMMAND} -E touch ${TEST_TIMESTAMP_FILE}
        DEPENDS ${TEST_TARGET_EXECUTABLE}
        COMMENT "Running ${TEST_TARGET_EXECUTABLE}")
endfunction()

################ General compilation flags

set(NPF_RUNTIME_FLAGS -pedantic -ansi -fstrict-aliasing -Wsign-conversion)
set(NPF_COMMON_FLAGS -Wall
                     -Wextra
                     -Wunreachable-code
                     -Wshadow
                     -Wcast-align
                     -Wcast-qual
                     -Wstrict-aliasing
                     -Wmissing-braces
                     -march=native)

################ Targets

add_subdirectory(unit_tests)
