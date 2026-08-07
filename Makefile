# nanoprintf top-level Makefile (POSIX)

# --- Configuration (override on command line) ---
CC      ?= cc
CXX     ?= c++
CFG     ?= Release
ARCH    ?= 64
SAN     ?= none
# Build only shard I of N flag combinations. Lets CI spread the conformance
# matrix over parallel jobs; 1/1 builds every combination.
SHARD   ?= 1/1
VERBOSE ?=

BUILD := build
GEN   := $(BUILD)/generated

# --- envy-managed packages ---
# Supplying DOCTEST_H opts out: envy is never invoked and nothing is downloaded, so
# DOCTEST_H and PYTHON3 are all the build needs. See README "Building without envy".
ENVY := ./bin/envy

ifneq ($(MAKECMDGOALS),clean)
  ifeq ($(origin DOCTEST_H),undefined)
    # Installed up front so a fresh clone running `make -j12` has every package before the
    # first rule fires, not one at a time as the shims are reached.
    ENVY_INSTALLED := $(shell $(ENVY) install && echo ok)
    ifneq ($(ENVY_INSTALLED),ok)
      $(error envy install failed)
    endif

    PYTHON3   ?= ./bin/python3
    DOCTEST_H := $(shell $(ENVY) -q product doctest_cpp_h)
    ifeq ($(DOCTEST_H),)
      $(error envy could not resolve doctest_cpp_h)
    endif
  else
    # Not ./bin/python3: that shim re-execs envy.
    PYTHON3 ?= python3
    ifeq ($(wildcard $(DOCTEST_H)),)
      $(error DOCTEST_H=$(DOCTEST_H) does not exist)
    endif
  endif

  # -isystem, not -I: doctest.h is not ours to keep clean under -Weverything -Werror.
  DOCTEST_INC := -isystem $(dir $(DOCTEST_H))
endif

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

# Globbed so a new test file needs no registration here, and so build.py can compile
# the same set from the same glob rather than a second hand-maintained copy.
UNIT_SRCS := $(sort $(wildcard tests/unit_*.cc))

UNIT_OBJS       := $(patsubst tests/%.cc,$(BUILD)/unit/%.o,$(UNIT_SRCS))
UNIT_LARGE_OBJS := $(patsubst tests/%.cc,$(BUILD)/unit_large/%.o,$(UNIT_SRCS))

# --- Header dependencies ---
NPF_H     := nanoprintf.h
TEST_HDRS := tests/unit_nanoprintf.h tests/npf_doctest.h $(DOCTEST_H) tests/unit_eg.inc tests/npf_f_paths.h

# ============================================================
# Top-level targets
# ============================================================

.PHONY: all conformance unit compile-only clean FORCE

all: conformance unit compile-only

# --- Config change detection ---
$(BUILD)/config.stamp: FORCE
	@mkdir -p $(BUILD)
	@echo '$(CC) $(CXX) $(CFG) $(ARCH) $(SAN) $(SHARD)' > $(BUILD)/config.stamp.tmp
	@cmp -s $(BUILD)/config.stamp.tmp $@ 2>/dev/null || cp $(BUILD)/config.stamp.tmp $@
	@rm -f $(BUILD)/config.stamp.tmp

# --- Conformance tests (recursive make) ---
$(GEN)/Makefile: tests/gen_tests.py $(BUILD)/config.stamp
	$(MSG) GEN conformance
	$(QUIET)$(PYTHON3) tests/gen_tests.py --output $(GEN) --cc "$(CC)" --cxx "$(CXX)" --arch $(ARCH) --sanitizer $(SAN) --shard $(SHARD)

conformance: $(GEN)/Makefile
	$(QUIET)$(MAKE) -C $(GEN) $(if $(filter 1,$(VERBOSE)),V=1)

# --- Doctest main (compiled once) ---
$(BUILD)/doctest_main.o: tests/doctest_main.cc $(TEST_HDRS) $(BUILD)/config.stamp
	@mkdir -p $(BUILD)
	$(MSG) CXX $<
	$(QUIET)$(CXX) $(CXXFLAGS) $(TEST_WARN) $(DOCTEST_INC) -c -o $@ $<

# --- Unit tests (LARGE=0) ---
$(BUILD)/unit/%.o: tests/%.cc $(NPF_H) $(TEST_HDRS) $(BUILD)/config.stamp
	@mkdir -p $(BUILD)/unit
	$(MSG) CXX $<
	$(QUIET)$(CXX) $(CXXFLAGS) $(TEST_WARN) $(DOCTEST_INC) $(UNIT_DEFS) -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -c -o $@ $<

$(BUILD)/unit_tests: $(UNIT_OBJS) $(BUILD)/doctest_main.o
	$(MSG) LINK $@
	$(QUIET)$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/unit_tests.timestamp: $(BUILD)/unit_tests
	$(MSG) RUN $<
	$(QUIET)./$< -m && touch $@

# --- Unit tests: large variant (LARGE=1) ---
$(BUILD)/unit_large/%.o: tests/%.cc $(NPF_H) $(TEST_HDRS) $(BUILD)/config.stamp
	@mkdir -p $(BUILD)/unit_large
	$(MSG) CXX $<
	$(QUIET)$(CXX) $(CXXFLAGS) $(TEST_WARN) $(DOCTEST_INC) $(UNIT_DEFS) -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1 -c -o $@ $<

$(BUILD)/unit_tests_large: $(UNIT_LARGE_OBJS) $(BUILD)/doctest_main.o
	$(MSG) LINK $@
	$(QUIET)$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/unit_tests_large.timestamp: $(BUILD)/unit_tests_large
	$(MSG) RUN $<
	$(QUIET)./$< -m && touch $@

unit: $(BUILD)/unit_tests.timestamp $(BUILD)/unit_tests_large.timestamp

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
# Everything under $(BUILD) except the package cache: refetching the toolchain is not
# what anyone means by `make clean`. Use `rm -rf $(BUILD)` for that.
clean:
	$(QUIET)if [ -d $(BUILD) ]; then \
	  find $(BUILD) -mindepth 1 -maxdepth 1 ! -name envy-cache -exec rm -rf {} +; \
	fi
