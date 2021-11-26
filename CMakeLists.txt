cmake_minimum_required(VERSION 3.15)
project(nanoprintf)

option(NPF_32BIT "Compile nanoprintf tests in 32-bit mode")
option(NPF_PALAND "Compile and run the mpaland printf test suite")

if (NPF_32BIT AND CMAKE_HOST_APPLE)
  message(FATAL_ERROR "Apple doesn't support 32-bit mode anymore.")
endif()

if (NPF_32BIT AND NOT MSVC)
  set(NPF_32BIT_FLAG -m32)
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${NPF_32BIT_FLAG})
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${NPF_32BIT_FLAG})
endif()

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

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND nanoprintf_common_flags -Weverything)
  list(APPEND nanoprintf_cxx_flags
       -Wno-old-style-cast
       -Wno-c++98-compat-pedantic
       -Wno-missing-prototypes
       -Wno-reserved-identifier)
endif()

set(nanoprintf_c_flags ${nanoprintf_common_flags})
set(nanoprintf_cxx_flags ${nanoprintf_common_flags} ${nanoprintf_cxx_flags})

################ Doctest

add_library(libdoctest_main OBJECT tests/doctest_main.cc)

function(npf_test name files)
  add_executable(${name} ${files})
  target_link_libraries(${name} libdoctest_main)

  if (NOT MSVC)
    # The tests use format strings that anger clang + gcc
    target_compile_options(${name} PRIVATE
                           ${nanoprintf_cxx_flags}
                           -Wno-format
                           -Wno-format-pedantic
                           -Wno-format-nonliteral)
  endif()

  # Set up a target that automatically runs + timestamps successful tests.

  set(timestamp "${CMAKE_CURRENT_BINARY_DIR}/${name}.timestamp")
  add_custom_target(run_${name} ALL DEPENDS ${timestamp})

  add_custom_command(OUTPUT ${timestamp}
                     COMMAND ${name} && ${CMAKE_COMMAND} -E touch ${timestamp}
                     DEPENDS ${name}
                     COMMENT "Running ${name}")
endfunction()

################ Language compilation tests

function(npf_compilation_c_test target)
  add_library(${target} tests/compilation_c.c)
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

          set(test_name "")
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
          set(compilation_test_name "npf_compile${test_name}_c")
          npf_compilation_c_test(${compilation_test_name})
          target_compile_definitions(
            ${compilation_test_name}
            PRIVATE
            NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=${fw}
            NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=${precision}
            NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=${large}
            NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=${float}
            NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=${wb})

          # Run conformance tests (c++)
          set(conformance_test_name "npf_conform${test_name}")
          npf_test(${conformance_test_name} tests/conformance.cc)
          target_compile_definitions(
            ${conformance_test_name}
            PRIVATE
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

# Test that nanoprintf compiles when no flags are set.
npf_compilation_c_test(npf_c_default_flags)

################ Static compilation test

add_executable(npf_static tests/static_nanoprintf.c tests/static_main.c)

################# Examples

add_executable(use_npf_directly
               examples/use_npf_directly/your_project_nanoprintf.cc
               examples/use_npf_directly/main.cc)

add_executable(wrap_npf
               examples/wrap_npf/your_project_printf.h
               examples/wrap_npf/your_project_printf.cc
               examples/wrap_npf/main.cc)

set(unit_test_files
    nanoprintf.h
    tests/unit_nanoprintf.c
    tests/unit_parse_format_spec.cc
    tests/unit_bufputc.cc
    tests/unit_ftoa_rev.cc
    tests/unit_itoa_rev.cc
    tests/unit_utoa_rev.cc
    tests/unit_fsplit_abs.cc
    tests/unit_snprintf.cc
    tests/unit_vpprintf.cc)

############### Unit tests

npf_test(unit_tests_normal_sized_formatters "${unit_test_files}")
target_compile_definitions(unit_tests_normal_sized_formatters
                           PRIVATE
                           NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0)

npf_test(unit_tests_large_sized_formatters "${unit_test_files}")
target_compile_definitions(unit_tests_large_sized_formatters
                           PRIVATE
                           NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1)

############## Paland conformance tests

if (NPF_PALAND)
  npf_test(paland_conformance tests/mpaland-conformance/paland.cc)
endif()
