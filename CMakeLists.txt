cmake_minimum_required(VERSION 3.15)
project(nanoprintf)

option(NPF_32BIT "Compile nanoprintf tests in 32-bit mode")
option(NPF_PALAND "Compile and run the mpaland printf test suite")
option(NPF_CLANG_ASAN "Compile and run tests with address sanitizer")
option(NPF_CLANG_UBSAN "Compile and run tests with undefined behavior sanitizer")

if (NPF_32BIT AND CMAKE_HOST_APPLE)
  message(FATAL_ERROR "Apple doesn't support 32-bit mode anymore.")
endif()

if ((NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND (NPF_CLANG_ASAN OR NPF_CLANG_UBSAN))
  message(FATAL_ERROR "Can only use asan/ubsan on Clang configurations.")
endif()

if (NPF_CLANG_ASAN AND NPF_CLANG_UBSAN)
  message(FATAL_ERROR "Can only use one of asan/ubsan per configuration.")
endif()

if (NPF_32BIT AND NOT MSVC)
  set(NPF_32BIT_FLAG -m32)
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${NPF_32BIT_FLAG})
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${NPF_32BIT_FLAG})
endif()

################ Common compile flags

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
  set(nanoprintf_common_flags /Wall /WX)
else()
  set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g3")
  set(CMAKE_C_FLAGS_RELEASE "-Os")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g3")
  set(CMAKE_CXX_FLAGS_RELEASE "-Os")

  set(nanoprintf_common_flags -pedantic -Wall -Wextra -Wundef -Werror)
  set(nanoprintf_link_flags "")

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND nanoprintf_common_flags -Weverything)
    if (NPF_CLANG_ASAN)
      list(APPEND nanoprintf_common_flags -fsanitize=address)
      list(APPEND nanoprintf_link_flags -fsanitize=address)
    elseif (NPF_CLANG_UBSAN)
      list(APPEND nanoprintf_common_flags -fsanitize=undefined)
      list(APPEND nanoprintf_link_flags -fsanitize=undefined)
    endif()
    if (CMAKE_HOST_APPLE)
      list(APPEND nanoprintf_common_flags -Wno-poison-system-directories)
    endif()
  else()
    list(APPEND nanoprintf_common_flags
         -Wconversion
         -Wshadow
         -Wfloat-equal
         -Wsign-conversion
         -Wswitch-enum
         -Wswitch-default)
  endif()
endif()

################ Doctest

add_library(libdoctest_main OBJECT tests/doctest_main.cc)
target_compile_options(libdoctest_main PRIVATE ${nanoprintf_common_flags})

function(npf_test name files)
  add_executable(${name} ${files})
    target_compile_definitions(${name} PRIVATE DOCTEST_CONFIG_SUPER_FAST_ASSERTS)
    target_compile_options(${name} PRIVATE ${nanoprintf_common_flags})
    target_link_options(${name} PRIVATE ${nanoprintf_link_flags})
    target_link_libraries(${name} libdoctest_main) # Doctest is slow, only build once.

  # Set up a target that automatically runs + timestamps successful tests.
  set(timestamp "${CMAKE_CURRENT_BINARY_DIR}/${name}.timestamp")
  add_custom_target(run_${name} ALL DEPENDS ${timestamp})
  add_custom_command(OUTPUT ${timestamp}
                     COMMAND ${name} -m && ${CMAKE_COMMAND} -E touch ${timestamp}
                     DEPENDS ${name}
                     COMMENT "Running ${name}")
endfunction()

################ Language compilation tests

function(npf_compilation_c_test target)
  add_library(${target} tests/compilation_c.c)
  target_compile_options(${target} PRIVATE ${nanoprintf_common_flags})
endfunction()

# Test every combination of compatible flags.
foreach(fw 0 1)
  foreach(precision 0 1)
    foreach(large 0 1)
      foreach(float 0 1)
        foreach(binary 0 1)
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
            if (binary EQUAL 1)
              string(APPEND test_name "_binary")
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
                NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=${binary}
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
                NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=${binary}
                NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=${wb})

            if (NPF_PALAND)
              set(paland_test_name "npf_paland${test_name}")
              npf_test(${paland_test_name} tests/mpaland-conformance/paland.cc)
                target_compile_definitions(
                  ${paland_test_name}
                  PRIVATE
                  NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=${fw}
                  NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=${precision}
                  NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=${large}
                  NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=${float}
                  NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=${binary}
                  NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=${wb})
            endif()
          endforeach()
        endforeach()
      endforeach()
    endforeach()
  endforeach()
endforeach()

# Test that nanoprintf compiles when no flags are set.
npf_compilation_c_test(npf_c_default_flags)

################ Static compilation test

add_executable(npf_static tests/static_nanoprintf.c tests/static_main.c)
  target_link_options(npf_static PRIVATE ${nanoprintf_link_flags})

################# Examples

add_executable(use_npf_directly
               examples/use_npf_directly/your_project_nanoprintf.cc
               examples/use_npf_directly/main.cc)
  target_link_options(use_npf_directly PRIVATE ${nanoprintf_link_flags})

add_executable(wrap_npf
               examples/wrap_npf/your_project_printf.h
               examples/wrap_npf/your_project_printf.cc
               examples/wrap_npf/main.cc)
  target_link_options(wrap_npf PRIVATE ${nanoprintf_link_flags})

add_executable(npf_include_multiple tests/include_multiple.c)
  target_compile_options(npf_include_multiple PRIVATE ${nanoprintf_common_flags})
  target_link_options(npf_include_multiple PRIVATE ${nanoprintf_link_flags})

############### Unit tests

set(unit_test_files
    nanoprintf.h
    tests/unit_parse_format_spec.cc
    tests/unit_binary.cc
    tests/unit_bufputc.cc
    tests/unit_ftoa_rev.cc
    tests/unit_itoa_rev.cc
    tests/unit_utoa_rev.cc
    tests/unit_fsplit_abs.cc
    tests/unit_snprintf.cc
    tests/unit_snprintf_safe_empty.cc
    tests/unit_snprintf_safe_trim.cc
    tests/unit_vpprintf.cc)

npf_test(unit_tests_normal_sized_formatters "${unit_test_files}")
  target_compile_definitions(unit_tests_normal_sized_formatters
                             PRIVATE
                             NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0)
  if (NPF_32BIT)
  target_compile_definitions(unit_tests_normal_sized_formatters
                             PRIVATE
                             NANOPRINTF_32_BIT_TESTS)
  endif()

npf_test(unit_tests_large_sized_formatters "${unit_test_files}")
  target_compile_definitions(unit_tests_large_sized_formatters
                             PRIVATE
                             NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1)
  if (NPF_32BIT)
  target_compile_definitions(unit_tests_normal_sized_formatters
                             PRIVATE
                             NANOPRINTF_32_BIT_TESTS)
  endif()
