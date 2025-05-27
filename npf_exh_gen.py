# Exhaustive test generator for NPF.
#
# Generates "exhaustive" test cases, all to be compared with the system printf
# (or another known-good implementation -- ie not against oracle strings).
# Generates the test code too, though that can be modified as needed.
# The test cases are generated as 'test_print' calls inside a 'main' function.
# Each test call comes with a format string and the proper arguments.
#
# "exhaustive" means that all possible combinations of specifiers/options are
# produced, subject to some constraints. The possible combinations can be
# customized below, in the "parameters" section.
# Also, it means that each combination is tested with different values for the
# arguments, covering the whole range of the input variables; however, there
# are only a handful of tested values; really testing every possible value would
# produce an astronomical number of test cases.
# Also note that the flags can combine freely. We do test all the "combinations",
# but not all the permutations (they are too many), nor all the (infinite)
# permutations with duplicates (also, duplicate flags are not specified by the
# standard).
# There are various possible types of combinations:
# + well-specified: these are covered
# + implementation-defined: (for instance, most of %a, all of %p, and nan/inf
#   for %a %e %f %g); these are bound to produce lots of false positives,
#   depending on the specifics of the reference printf that we test against.
#   Only some of these are enabled by default, below.
# + gray areas: things not mentioned by the standard (duplicated flags; '+' for
#   unsigned); some of these are fairly "straightforward" -- eg '+' can be
#   ignored for unsigned -- while others are more prone to different behavior in
#   different implementations -- eg a width for %% might be honored, ignored, or
#   rejected.
#   Not all of these are enabled by default.
# + ill-formed inputs: still in the "gray area", but bound to cause even more
#   problems, like a NULL format string, a NULL input string, a '\0' char, or a
#   NULL %n target, or a width/precision literal that overflows (int).
# + undetectable UB: these can easily cause crashes or serious misbheavior, and
#   cannot be detected by printf. eg mismatching format/parameters, invalid
#   pointer.
#
# The tests also include some checks against buffer/writeback overrun.
# Serious malfunctions of printf cannot be detected with certainy -- they may
# fail silently, fail with an appropriate message, produce a crash, etc.
# Only specifiers/modifiers relevant to NPF are handled.
#
# The terminology used here is the same as the standard one.
# The format string consists of literal characters and/or "conversion
# specifications". Each specification has this format:
#   %<flags><width><precision><length_modifier><conversion_specifier>
# All but the 

import itertools
import json
import os

# These should not be changed
categs = {
    "a": ["a", "A"],
    "c": ["c"],
    "f": ["e", "E", "f", "F", "g", "G"],
    "i": ["d", "i"],
    "k": ["%"],
    "n": ["n"],
    "p": ["p"],
    "s": ["s"],
    "u": ["b", "B", "o", "u", "x", "X"],
    "": [""], # literal string
}

### PARAMETERS ###

# NOTE: out_path descendants will be deleted before generation, unless it is one of ["", ".", ".."]
out_path = "exh_tests"
out_file = "npf_exh_test"
out_file_ext = "c"
one_file_per_specifier = True

npf_path = "../"

# ref: None to use system printf
# include: all the code necessary to use such a printf (include, options, etc.)
ref_vsnprintf = None
code_for_ref_vsnprintf = None

# add 'if' conditions linked to the npf options, to skip tests that are not meaningful in the current configuration
add_option_guards = True

npf_opts = {
    "NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS" : 1         ,
    "NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS"   : 1         ,
    "NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"       : 1         ,
    "NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS"       : 1         ,
    "NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS"       : 1         ,
    "NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS"      : 1         ,
    "NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS"   : 1         ,
    "NANOPRINTF_USE_ALT_FORM_FLAG"                 : 1         ,
    "NANOPRINTF_CONVERSION_BUFFER_SIZE"            : 23        ,
    "NANOPRINTF_CONVERSION_FLOAT_TYPE"             : "uint64_t",
}

# To check whether a specifier causes problems to the parsing of the rest of the
# format string, or to the fetching of subsequent arguments, we can specify a
# suffix ([string, [args]) that would likely fail in that case.
# Use an empty string as the first item to disable this.
# To avoid problems, the suffix should contain only literal chars, and specifiers
# and options that are always supported in all NPF configurations.
#suffix = ["", []] # nothing
#suffix = ["@", []] # parsing only
suffix = ["$%c", ["'@'"]] # parsing + fetching

specs = [
  "" ,
#  "a", "A", # most of %a is IB; only +-0.0 is univocally specified
  "b", "B",
  "c",
  "d", "i",
#  "e", "E",
  "f", "F",
#  "g", "G",
  "n",
  "o",
#  "p", # %p is completely IB
  "s",
  "u",
  "x", "X",
  "%",
]

# Options that are mutually exclusive must list all the possibilities; if none,
# there must be a single "" item.
# Options that are cumulative (ie flags) must list all the possibilities; if
# none, the list must be empty
modifs = {}
modifs["i"] = [
  "hh",
  "h" ,
  ""  ,
  "l" ,
  "ll",
  "j" ,
  "z" ,
  "t" ,
]
modifs["f"] = [
  "" ,
  "l",
  "L",
]
modifs["a"] = modifs["f"].copy()
modifs["c"] = [""]
modifs["k"] = [""]
modifs["n"] = modifs["i"].copy()
modifs["p"] = [""]
modifs["s"] = [""]
modifs["u"] = modifs["i"].copy()
modifs[""] = [""]

flags = {}
flags["i"] = [
  " ",
  "+",
  "-",
  "#",
  "0",
]
flags["f"] = flags["i"].copy()
flags["a"] = flags["f"].copy()
flags["c"] = [
  " ", # meaningless
  "+", # meaningless
  "-",
  "#", # meaningless
  "0",
]
flags["k"] = []
flags["n"] = []
flags["p"] = [
  " ",
  "+",
  "-",
  "#",
  "0",
]
flags["s"] = [
  " ", # meaningless
  "+", # meaningless
  "-",
  "#", # meaningless
  "0",
]
flags["u"] = [
  " ", # meaningless
  "+", # meaningless
  "-",
  "#",
  "0",
]
flags[""] = []

widths = {}
widths["i"] = [
  "" ,
  "n",
  "*",
]
widths["a"] = widths["i"].copy()
widths["c"] = widths["i"].copy()
widths["f"] = widths["i"].copy()
widths["k"] = [""]
widths["n"] = [""]
widths["p"] = widths["i"].copy()
widths["s"] = widths["i"].copy()
widths["u"] = widths["i"].copy()
widths[""] = [""]

precs = {}
precs["i"] = [
  "" ,
  "n",
  "*",
]
precs["a"] = precs["i"].copy()
precs["c"] = [""]
precs["f"] = precs["i"].copy()
precs["k"] = [""]
precs["n"] = [""]
precs["p"] = precs["i"].copy()
precs["s"] = precs["i"].copy()
precs["u"] = precs["i"].copy()
precs[""] = [""]


# do not modify this
def qs(s): # string -> quoted string
    s = json.dumps(s) # escape quotes, backslash, non-printable chars, etc.
    s = s.replace("?", "\\?") # avoid problems with trigraphs
    return s

# do not modify this
def escape_fmt_str(s):
    s = s.replace("%", "%%") # must escape this for the format string
    return s

# Test values tailored to the spec/modifier, to check edge cases like 0 and the numeric limits
# To avoid conversion errors, all values must be represented as strings, which
# will be printed verbatim.
# String themselves must be represented as their literal constant string, ie
# double-quoted; this can be hand-written, or achieved by quoting a string with
# qs()
def get_values(categ, modif):
    # we mostly ignore "L", and treat all doubles as f64
    if categ == "a":
        if modif == "L":
            return ["0.0L", "-0.0L", "1.0L", "0x1.fffffffffffffp1023L", "-0x1p-1074L"]
        else:
            return ["0.0", "-0.0", "1.0", "0x1.fffffffffffffp1023", "-0x1p-1074"]
    if categ == "c":
        return ["'~'"]
    if categ == "f":
        if modif == "L":
            return ["0.0L", "-0.0L", "1.0L", "-1.234e-4L", "9.5e10L"]
        else:
            return ["0.0", "-0.0", "1.0", "-1.234e-4", "9.5e10"]
    if categ == "i":
        if modif == "hh":
            return ["(signed char)0", "1", "SCHAR_MIN", "SCHAR_MAX", "INT_MIN", "INT_MAX"]
        if modif == "h":
            return ["(short)0", "1", "SHRT_MIN", "SHRT_MAX", "INT_MIN", "INT_MAX"]
        if modif == "":
            return ["0", "1", "INT_MIN", "INT_MAX"]
        if modif == "l":
            return ["0l", "1l", "LONG_MIN", "LONG_MAX"]
        if modif == "ll":
            return ["0ll", "1ll", "LLONG_MIN", "LLONG_MAX"]
        if modif == "j":
            return ["(intmax_t)0", "(intmax_t)1", "INTMAX_MIN", "INTMAX_MAX"]
        if modif == "z":
            return ["(ptrdiff_t)0", "(ptrdiff_t)1", "PTRDIFF_MIN", "PTRDIFF_MAX"] # no standard ssize_t, reuse ptrdiff_t
        if modif == "t":
            return ["(ptrdiff_t)0", "(ptrdiff_t)1", "PTRDIFF_MIN", "PTRDIFF_MAX"]
    if categ == "k":
        return [None]
    if categ == "n":
        return [None]
    if categ == "p":
            return ["(void *)0", "(void *)1", "(void *)-1"]
    if categ == "s":
            return [qs(""), qs("abc"), qs("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" * 5)]
    if categ == "u":
        if modif == "hh":
            return ["(unsigned char)0", "1u", "UCHAR_MAX", "UINT_MAX"]
        if modif == "h":
            return ["(unsigned short)0", "1u", "USHRT_MAX", "UINT_MAX"]
        if modif == "":
            return ["0u", "1u", "UINT_MAX"]
        if modif == "l":
            return ["0ul", "1ul", "ULONG_MAX"]
        if modif == "ll":
            return ["0ull", "1ull", "ULLONG_MAX"]
        if modif == "j":
            return ["(uintmax_t)0", "(uintmax_t)1", "UINTMAX_MAX"]
        if modif == "z":
            return ["(size_t)0", "(size_t)1", "SIZE_MAX"]
        if modif == "t":
            return ["(size_t)0", "(size_t)1", "SIZE_MAX"] # no standard uptrdiff_t, reuse size_t
    if categ == "":
        return ["", "abcdefg", "".join([chr(i) for i in range(1, 127+1)])]
    raise ValueError("get_values: unknown categ '{}'".format(categ))


# to properly check %n, we must print a certain number of characters before it (not always 0)
def get_prefixes(categ, modif):
    if categ == "n":
        if modif == "hh":
            return ["", "@", "@" * 127]
        else:
            return ["", "@", "@" * 200]
    return [""]


def get_widths(categ, modif, width):
    if width == "n":
        return (["1", "5", "50"], [None])
    elif width == "*":
        return (["*"], ["-50", "-5", "-1", "0", "1", "5", "50"])
    else:
        return ([""], [None])


def get_precs(categ, modif, prec):
    if prec == "n":
        if categ in ["a", "f"]:
            return (["0", "1", "14"], [None])
        else:
            return (["0", "1", "5", "50"], [None])
    elif prec == "*":
        if categ in ["a", "f"]:
            return (["*"], ["-14", "-1", "0", "1", "14"])
        else:
            return (["*"], ["-50", "-5", "-1", "0", "1", "5", "50"])
    else:
        return ([""], [None])


### END OF PARAMETERS ###

####


def rm_dir_contents(path):
    with os.scandir(path) as items:
        for item in items:
            if item.is_dir() and not item.is_symlink():
                shutil.rmtree(item.path)
            else:
                os.remove(item.path)


def remove_item(lst, item):
    try:
        lst.remove(item)
    except ValueError:
        pass


def remove_items(lists, items):
    for lst in lists:
        for item in items:
            remove_item(lst, item)


if not npf_opts["NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS"]:
    remove_items(
        widths.values(),
        ["n", "*"]
        )
    remove_items(
        flags.values(),
        ["-", "0"]
        )
if not npf_opts["NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS"]:
    remove_items(
        precs.values(),
        ["n", "*"]
        )
if not npf_opts["NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"]:
    remove_items([specs], categs["a"] + categs["b"])
if not npf_opts["NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS"]:
    remove_items(
        modifs.values(),
        ["l", "ll", "j", "z", "t"]
        )
if not npf_opts["NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS"]:
    remove_items(
        modifs.values(),
        ["hh", "h"]
        )
if not npf_opts["NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS"]:
    remove_items(
        [specs],
        ["b", "B"]
        )
if not npf_opts["NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS"]:
    remove_items(
        [specs],
        ["n"]
        )
if not npf_opts["NANOPRINTF_USE_ALT_FORM_FLAG"]:
    remove_items(
        flags.values(),
        ["#"]
        )


# superset of {items}
# This actually returns a list, and each element is the string concatenation of
# the elements, rather than a set of elements
def superset(items):
    sets = [""]
    for x in items:
        sets.extend([s + x for s in sets])
    return sets


# all the permutations of the superset of {items}
# This actually returns a list, and each element is the string concatenation of
# the elements
def superperms(items):
    sets = superset(items)
    perms = [itertools.permutations(it) for it in sets]
    perms = [[pp for pp in p] for p in perms]
    perms = [x for y in perms for x in y]
    perms = ["".join(list(p)) for p in perms]
    return perms


def find_categ(spec):
    for k in categs.keys():
        if spec in categs[k]:
            return k
    raise ValueError("Unknown spec '{}'".format(spec))


def get_opt_guards(spec, flags_s, width_s, prec_s, mod_s):
    res = []
    if spec in ["a", "A", "e", "E", "f", "F", "g", "G"]:
        res.append("opt_f")
    if spec in ["b", "B"]:
        res.append("opt_b")
    if spec == "n":
        res.append("opt_n")
    if "#" in flags_s:
        res.append("opt_alt")
    if width_s != "":
        res.append("opt_width")
    if prec_s != "":
        res.append("opt_prec")
    if mod_s in ["h", "hh"]:
        res.append("opt_small")
    if mod_s in ["ll", "j", "z", "t"]:
        res.append("opt_large")
    return res


def emit_spec(spec):
  categ = find_categ(spec)
  all_flags = flags[categ]
  flag_comb = superset(all_flags) # all possible orderless combinations of flags, no repetitions
  #flag_comb = superperms(all_flags) # all possible ordered combinations of flags, no repetitions -- these are a *lot*
  for modif in modifs[categ]:
    prefixes = get_prefixes(categ, modif)
    values = get_values(categ, modif)
    for pref in prefixes:
      for prec in precs[categ]:
        (prec_ss, prec_vv) = get_precs(categ, modif, prec)
        for prec_s in prec_ss:
          prec_s = prec_s if prec_s == "" else "." + prec_s
          for prec_v in prec_vv:
            for width in widths[categ]:
              (width_ss, width_vv) = get_widths(categ, modif, width)
              for width_s in width_ss:
                for width_v in width_vv:
                  for f in flag_comb:
                    for v in values:
                      if categ == "":
                        fmt = escape_fmt_str(v)
                        args = []
                      else:
                        fmt = pref + "%" + f + width_s + prec_s + modif + spec
                        args = [width_v, prec_v, v]
                        args = [a for a in args if a is not None]
                      if add_option_guards:
                        guards = get_opt_guards(spec, f, width_ss, prec_ss, modif)
                      else:
                        guards = []
                      yield (fmt, args, guards)


def emit_specs(fpath, specs, tag):
    main_fn = "main" if tag is None else f"test_exh_{tag}"
    out = []
    n_tests = 0
    for spec in specs:
        for v in emit_spec(spec):
            fmt = [v[0]]
            args = [v[1]] if v[1] != [] else []
            guards = v[2]
            
            if suffix[0] != "":
                fmt += [suffix[0]]
                args += [suffix[1]]
            fmt = [" ".join([qs(f) for f in fmt])]
            args = [it for lst in args for it in lst]
            args = ", ".join(fmt + args)
            test_case = "  test_print(" + args + ");"
            
            if len(guards) != 0:
                guards_cond = "  if ({}) ".format(" && ".join(guards))
                test_case = guards_cond + test_case
            out.append(test_case)
            n_tests += 1

    pre = r"""#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

"""
    pre += "\n".join(["#define {}    {}".format(k, npf_opts[k]) for k in npf_opts.keys()])
    pre += r"""
#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_VISIBILITY_STATIC
"""
    pre += f"#include \"{npf_path}nanoprintf.h\"\n"
    pre += (code_for_ref_vsnprintf + "\n" if code_for_ref_vsnprintf is not None else "")
    pre += r"""

typedef struct
{
  char pad_pre[64];
  signed char         n_hhi;
  signed short        n_hi;
  signed int          n_i;
  signed long         n_li;
  signed long long    n_lli;
  intmax_t            n_ji;
  size_t              n_zi;
  ptrdiff_t           n_ti;
  char pad_post[64];
} p_data_t;
static p_data_t p_data;

typedef struct
{
  char pad_pre[1024];
  char data[8192];
  char pad_post[1024];
} buffer_t;
static buffer_t buffer_ref;
static buffer_t buffer_npf;

#if !defined(_MSC_VER)
__attribute__((unused))
#endif
static
double npf_u64_to_dbl(uint64_t v) {
  double d;
  memcpy(&d, &v, 8);
  return d;
}

#define test_print(...)    test_print_helper(__LINE__, __VA_ARGS__)
#define REQUIRE(cond)    SUB_REQUIRE(__LINE__, cond)
#define SUB_REQUIRE(line, cond)               SUB_REQUIRE_HELPER(line, __LINE__, cond, #cond)
#define SUB_REQUIRE_HELPER(line, subline, cond, cond_str)    do { if (!(cond)) { FAIL(line, subline, cond_str); } } while (0)
""" + "#define N_TESTS    {}l".format(n_tests) + r"""
static
const char *cmp_p_data(const p_data_t *p_ref, const p_data_t *p_npf)
{
#define NPF_CMP_P_DATA(suffix)   do { if (p_ref->n_##suffix != p_npf->n_##suffix)  { return #suffix; } } while(0)
  NPF_CMP_P_DATA(hhi);
  NPF_CMP_P_DATA(hi );
  NPF_CMP_P_DATA(i  );
  NPF_CMP_P_DATA(li );
  NPF_CMP_P_DATA(lli);
  NPF_CMP_P_DATA(ji );
  NPF_CMP_P_DATA(zi );
  NPF_CMP_P_DATA(ti );
#undef NPF_CMP_P_DATA
  return NULL;
}

// returns -1 if equal; >= 0 to indicate the byte-index of the diff
static
int npf_cmp_bytes(const void *a, const void *b, int n) {
  // First check for equality with branchless code (the expected path
  // is that the two arrays are identical)
  // If something differs, go back to search where the difference is
  const char *aa = (const char *)a;
  const char *bb = (const char *)b;
  unsigned diff = 0;
  for (int i = 0; i < n; ++i) {
    diff |= (unsigned)aa[i] ^ (unsigned)bb[i];
  }
  if (diff == 0) {
    return -1;
  }
  for (int i = 0; i < n; ++i) {
    if(aa[i] != bb[i]) {
      return i;
    }
  }
  return n; // should be impossible
}

// returns -1 if equal; >= 0 to indicate the byte-index of the diff
static
int npf_cmp_str(const char *a, const char *b, int n) {
  for (int i = 0; i < n; ++i) {
    if (a[i] != b[i]) {
      return i;
    }
    if(a[i] == '\0') {
      break;
    }
  }
  return -1;
}

static int cnt;
static int fails;
static const long max_fails = 100;

static
void test_print_helper(int line, const char *fmt, ...) {
#if 0
  {
    va_list args;
    va_start(args, fmt);
    int n = """ + (ref_vsnprintf if ref_vsnprintf is not None else "vsnprintf") + r"""(buffer_ref.data, sizeof(buffer_ref.data), fmt, args);
    va_end(args, fmt);
    p_data_ref = p_data;
    printf("%4d: |%s|\n", n, buffer_ref.data);
  }
#else
  int n_ref;
  int n_npf;
  
  p_data_t p_data_ref = {0};
  p_data_t p_data_npf = {0};
  
  memset(&p_data_ref, 0xA6, sizeof(p_data_ref));
  memset(&p_data_npf, 0xA6, sizeof(p_data_npf));
  
  memset(&buffer_ref, 0xA6, sizeof(buffer_ref));
  memset(&buffer_npf, 0xA6, sizeof(buffer_npf));
  
  {
    va_list args;
    va_start(args, fmt);
    n_ref = """ + (ref_vsnprintf if ref_vsnprintf is not None else "vsnprintf") + r"""(buffer_ref.data, sizeof(buffer_ref.data), fmt, args);
    va_end(args);
    p_data_ref = p_data;
  }
  {
    va_list args;
    va_start(args, fmt);
    n_npf = npf_vsnprintf(buffer_npf.data, sizeof(buffer_npf.data), fmt, args);
    va_end(args);
    p_data_npf = p_data;
  }
  int no_err = 0;
  const char* p_data_mismatch = cmp_p_data(&p_data_ref, &p_data_npf);
  int buffer_mismatch_idx = npf_cmp_bytes(&buffer_ref, &buffer_npf, sizeof(buffer_ref));
  int p_data_mismatch_idx = npf_cmp_bytes(&p_data_ref, &p_data_npf, sizeof(p_data_ref));
  int str_mismatch_idx = npf_cmp_str(buffer_ref.data, buffer_npf.data, sizeof(buffer_ref.data));
  int len_npf = (int)strnlen(buffer_npf.data, sizeof(buffer_npf.data));
  if (len_npf >= (int)sizeof(buffer_npf.data)) {
    printf("FAIL (len too high) @%d: %d\n", line, len_npf);
  } else if (str_mismatch_idx >= 0) {
    printf("FAIL (str) @%d:\n\tref %6d |%s|\n\tnpf %6d |%s|\n\t%*s\n", line, n_ref, buffer_ref.data, n_npf, buffer_npf.data, str_mismatch_idx+12+1, "^");
  } else if (n_ref != n_npf) {
    printf("FAIL (len) @%d:\n\tref %6d |%s|\n\tnpf %6d |%s|\n", line, n_ref, buffer_ref.data, n_npf, buffer_npf.data);
  } else if (len_npf != n_npf) {
    printf("FAIL (len mismatch) @%d:\n\tref %6d |%s|\n\tnpf %6d |%s|\n", line, n_ref, buffer_ref.data, n_npf, buffer_npf.data);
  } else if (buffer_mismatch_idx >= 0) {
    printf("FAIL (buffer corruption @%d): byte #%d\n", line, buffer_mismatch_idx);
  } else if (p_data_mismatch != NULL) {
    printf("FAIL (p_data mismatch in %s) @%d\n", p_data_mismatch, line);
  } else if (p_data_mismatch_idx >= 0) {
    printf("FAIL (p_data corruption) @%d: byte #%d\n", line, p_data_mismatch_idx);
  } else {
    no_err = 1;
  }
  if (!no_err) {
    fflush(stdout);
    ++fails;
    if (fails >= max_fails) {
      printf("Too many failures, abort.\n");
      exit(-1);
    }
  }
  ++cnt;
  if ((cnt % 25000) == 0) {
    printf("Test %d / %d\n", cnt, N_TESTS);
    fflush(stdout);
  }
#endif
}

int """ + main_fn + r"""(void) {
"""
    if tag is None:
        pre += r"""  printf("TEST EXH\n");
"""
    else:
        pre += r"""  printf("TEST EXH """ + tag + r"""\n");
"""

    if add_option_guards:
        pre += """
  const int opt_width  = (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1); (void)opt_width;
  const int opt_prec   = (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS   == 1); (void)opt_prec ;
  const int opt_f      = (NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS       == 1); (void)opt_f    ;
  const int opt_large  = (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS       == 1); (void)opt_large;
  const int opt_small  = (NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS       == 1); (void)opt_small;
  const int opt_b      = (NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS      == 1); (void)opt_b    ;
  const int opt_n      = (NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS   == 1); (void)opt_n    ;
  const int opt_alt    = (NANOPRINTF_USE_ALT_FORM_FLAG                 == 1); (void)opt_alt  ;

"""

    post = r"""
  printf("Done.\n");
  return fails;
}
"""
    out = "\n".join(out)

    with open(fpath, 'w', encoding="utf-8") as f:
        f.write(pre)
        f.write(out)
        f.write(post)
    
    return n_tests


def spec_to_tag(spec):
    if spec == "%":
        return "k"
    if spec == "":
        return "q"
    if spec.isupper():
        return spec.lower() * 2
    return spec


def generate_out_main(specs):
    out = ""
    out += """#include <stdio.h>

"""
    for spec in specs:
        tag = spec_to_tag(spec)
        out += f"extern int test_exh_{tag}(void);\n"
    out += r"""
int main(void) {
  int errs = 0;
"""
    for spec in specs:
        tag = spec_to_tag(spec)
        out += f"  errs += test_exh_{tag}(); printf(\"****\\n\");\n"
    out += r"""
  printf("Total: %d fails.\n", errs);
  printf("All done.\n");
  return errs;
}
"""
    
    return out

if out_path not in ["", ".", ".."]:
    if os.path.isdir(out_path):
        rm_dir_contents(out_path)
    else:
        os.mkdir(out_path)

n_tests_total = 0
if one_file_per_specifier:
    for spec in specs:
        tag = spec_to_tag(spec)
        fname = f"{out_file}_{tag}.{out_file_ext}"
        print("Generating {} ... ".format(fname))
        fpath = os.path.join(out_path, fname)
        n = emit_specs(fpath, [spec], tag)
        n_tests_total += n
    fname = f"{out_file}.{out_file_ext}"
    print("Generating {} ... ".format(fname))
    fpath = os.path.join(out_path, fname)
    out_main = generate_out_main(specs)
    with open(fpath, 'w', encoding="utf-8") as f:
        f.write(out_main)
else:
    fname = f"{out_file}.{out_file_ext}"
    print("Generating {}... ".format(fname))
    fpath = os.path.join(out_path, fname)
    n = emit_specs(fpath, specs, None)
    n_tests_total += n

print("Done. Generated {} test cases.".format(n_tests_total))