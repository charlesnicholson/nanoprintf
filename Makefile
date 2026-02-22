# nanoprintf top-level Makefile (POSIX)

# --- Configuration (override on command line) ---
CC      ?= cc
CXX     ?= c++
CFG     ?= Release
ARCH    ?= 64
SAN     ?= none
VERBOSE ?=
PYTHON3 ?= $(shell command -v python3.13 2>/dev/null || command -v python3 2>/dev/null || echo python3)

BUILD := build

# --- Verbosity ---
ifeq ($(VERBOSE),1)
  QUIET =
  MSG   = @true
else
  QUIET = @
  MSG   = @printf '  %-6s %s\n'
endif

# --- Compiler detection ---
COMPILER_ID := $(shell $(CC) --version 2>&1)
IS_CLANG    := $(findstring clang,$(COMPILER_ID))
UNAME_S     := $(shell uname -s)
IS_APPLE    := $(findstring Darwin,$(UNAME_S))

# --- Configuration-dependent flags ---
ifeq ($(CFG),Debug)
  OPT_FLAGS := -O0 -g3
else ifeq ($(CFG),RelWithDebInfo)
  OPT_FLAGS := -Os -g3
else
  OPT_FLAGS := -Os
endif

ifeq ($(ARCH),32)
  ARCH_FLAG := -m32
else
  ARCH_FLAG :=
endif

ifeq ($(SAN),asan)
  SAN_FLAGS := -fsanitize=address
else ifeq ($(SAN),ubsan)
  SAN_FLAGS := -fsanitize=undefined
else
  SAN_FLAGS :=
endif

# --- Warning flags ---
WARN_FLAGS := -pedantic -Wall -Wextra -Wundef -Werror

ifneq ($(IS_CLANG),)
  WARN_FLAGS += -Weverything
  ifneq ($(IS_APPLE),)
    WARN_FLAGS += -Wno-poison-system-directories
  endif
else
  WARN_FLAGS += -Wconversion -Wshadow -Wfloat-equal -Wsign-conversion \
                -Wswitch-enum -Wswitch-default
endif

# --- Assembled flags ---
CFLAGS   := $(WARN_FLAGS) $(OPT_FLAGS) $(ARCH_FLAG) $(SAN_FLAGS) -std=c17
CXXFLAGS := $(WARN_FLAGS) $(OPT_FLAGS) $(ARCH_FLAG) $(SAN_FLAGS) -std=c++20
LDFLAGS  := $(ARCH_FLAG) $(SAN_FLAGS)

# --- Test-specific warning suppressions ---
ifneq ($(IS_CLANG),)
  TEST_WARN := -Wno-c++98-compat-pedantic -Wno-format -Wno-format-nonliteral \
               -Wno-format-pedantic -Wno-format-security -Wno-format-zero-length \
               -Wno-missing-prototypes -Wno-old-style-cast -Wno-padded \
               -Wno-unsafe-buffer-usage-in-libc-call -Wno-unused-function
else
  TEST_WARN := -Wno-format -Wno-format-overflow -Wno-format-security \
               -Wno-format-zero-length -Wno-old-style-cast -Wno-unused-function
endif

# --- Unit test definitions ---
UNIT_DEFS := -DNANOPRINTF_USE_ALT_FORM_FLAG=1 -DDOCTEST_CONFIG_SUPER_FAST_ASSERTS

ifeq ($(ARCH),32)
  UNIT_DEFS += -DNANOPRINTF_32_BIT_TESTS
endif

UNIT_SRCS := tests/unit_parse_format_spec.cc \
             tests/unit_binary.cc \
             tests/unit_bufputc.cc \
             tests/unit_ftoa_nan.cc \
             tests/unit_ftoa_rev.cc \
             tests/unit_ftoa_rev_08.cc \
             tests/unit_ftoa_rev_16.cc \
             tests/unit_ftoa_rev_32.cc \
             tests/unit_ftoa_rev_64.cc \
             tests/unit_utoa_rev.cc \
             tests/unit_snprintf.cc \
             tests/unit_snprintf_safe_empty.cc \
             tests/unit_vpprintf.cc

UNIT_NORMAL_OBJS := $(patsubst tests/%.cc,$(BUILD)/unit_normal/%.o,$(UNIT_SRCS))
UNIT_LARGE_OBJS  := $(patsubst tests/%.cc,$(BUILD)/unit_large/%.o,$(UNIT_SRCS))

# --- Header dependencies ---
NPF_H     := nanoprintf.h
TEST_HDRS := tests/unit_nanoprintf.h tests/npf_doctest.h tests/doctest.h

# ============================================================
# Top-level targets
# ============================================================

.PHONY: all conformance unit compile-only clean FORCE

all: conformance unit compile-only

# --- Config change detection ---
$(BUILD)/config.stamp: FORCE
	@mkdir -p $(BUILD)
	@echo '$(CC) $(CXX) $(CFG) $(ARCH) $(SAN)' > $(BUILD)/config.stamp.tmp
	@cmp -s $(BUILD)/config.stamp.tmp $@ 2>/dev/null || cp $(BUILD)/config.stamp.tmp $@
	@rm -f $(BUILD)/config.stamp.tmp

# --- Conformance tests (recursive make) ---
tests/generated/Makefile: tests/gen_tests.py $(BUILD)/config.stamp
	$(MSG) GEN conformance
	$(QUIET)$(PYTHON3) tests/gen_tests.py --cc "$(CC)" --cxx "$(CXX)" --arch $(ARCH) --sanitizer $(SAN)

conformance: tests/generated/Makefile
	$(QUIET)$(MAKE) -C tests/generated $(if $(filter 1,$(VERBOSE)),V=1)

# --- Doctest main (compiled once) ---
$(BUILD)/doctest_main.o: tests/doctest_main.cc $(TEST_HDRS) $(BUILD)/config.stamp
	@mkdir -p $(BUILD)
	$(MSG) CXX $<
	$(QUIET)$(CXX) $(CXXFLAGS) $(TEST_WARN) -c -o $@ $<

# --- Unit tests: normal variant (LARGE=0) ---
$(BUILD)/unit_normal/%.o: tests/%.cc $(NPF_H) $(TEST_HDRS) $(BUILD)/config.stamp
	@mkdir -p $(BUILD)/unit_normal
	$(MSG) CXX $<
	$(QUIET)$(CXX) $(CXXFLAGS) $(TEST_WARN) $(UNIT_DEFS) -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -c -o $@ $<

$(BUILD)/unit_tests_normal: $(UNIT_NORMAL_OBJS) $(BUILD)/doctest_main.o
	$(MSG) LINK $@
	$(QUIET)$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/unit_tests_normal.timestamp: $(BUILD)/unit_tests_normal
	$(MSG) RUN $<
	$(QUIET)./$< -m && touch $@

# --- Unit tests: large variant (LARGE=1) ---
$(BUILD)/unit_large/%.o: tests/%.cc $(NPF_H) $(TEST_HDRS) $(BUILD)/config.stamp
	@mkdir -p $(BUILD)/unit_large
	$(MSG) CXX $<
	$(QUIET)$(CXX) $(CXXFLAGS) $(TEST_WARN) $(UNIT_DEFS) -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1 -c -o $@ $<

$(BUILD)/unit_tests_large: $(UNIT_LARGE_OBJS) $(BUILD)/doctest_main.o
	$(MSG) LINK $@
	$(QUIET)$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/unit_tests_large.timestamp: $(BUILD)/unit_tests_large
	$(MSG) RUN $<
	$(QUIET)./$< -m && touch $@

unit: $(BUILD)/unit_tests_normal.timestamp $(BUILD)/unit_tests_large.timestamp

# --- Compile-only targets ---
compile-only: $(BUILD)/npf_static $(BUILD)/npf_include_multiple \
              $(BUILD)/use_npf_directly $(BUILD)/wrap_npf

$(BUILD)/npf_static: tests/static_nanoprintf.c tests/static_main.c $(NPF_H) $(BUILD)/config.stamp
	$(MSG) CC $@
	$(QUIET)$(CC) -std=c17 $(OPT_FLAGS) $(ARCH_FLAG) $(SAN_FLAGS) -o $@ tests/static_nanoprintf.c tests/static_main.c

$(BUILD)/npf_include_multiple: tests/include_multiple.c $(NPF_H) $(BUILD)/config.stamp
	$(MSG) CC $@
	$(QUIET)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BUILD)/use_npf_directly: examples/use_npf_directly/your_project_nanoprintf.cc \
                           examples/use_npf_directly/main.cc $(NPF_H) $(BUILD)/config.stamp
	$(MSG) CXX $@
	$(QUIET)$(CXX) -std=c++20 $(OPT_FLAGS) $(ARCH_FLAG) $(SAN_FLAGS) -o $@ \
		examples/use_npf_directly/your_project_nanoprintf.cc examples/use_npf_directly/main.cc

$(BUILD)/wrap_npf: examples/wrap_npf/your_project_printf.cc examples/wrap_npf/main.cc \
                   examples/wrap_npf/your_project_printf.h $(NPF_H) $(BUILD)/config.stamp
	$(MSG) CXX $@
	$(QUIET)$(CXX) -std=c++20 $(OPT_FLAGS) $(ARCH_FLAG) $(SAN_FLAGS) -o $@ \
		examples/wrap_npf/your_project_printf.cc examples/wrap_npf/main.cc

# --- Clean ---
clean:
	rm -rf $(BUILD) tests/generated
