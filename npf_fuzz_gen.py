# Fuzzy test generator for NPF.
#
# Generates N test cases, all to be compared with the system printf (or another
# known-good implementation -- ie not against oracle strings).
# Generates the test code too, though that can be modified as needed.
# The test cases are generated as 'test_print' calls inside a 'main' function.
# Each test call comes with a format string and the proper arguments.
# Only sensible format strings are produced; some 'useless' ones are produced
# too (eg with redundant or conflicting flags), but no unparsable strings are
# produced. Calls with parameters not matching the specifiers in the format
# string are never produced either.
# Each test case includes up to 'max_specs' "chunks" in the format string --
# each chunk is either a sequence of literal characters, or a valid format
# specifier. Each specifier can have flags, width, precision, modifiers --
# modifiers etc. that are illegal for a specific conversion specifier are never
# generated.
# Dynamic width/precision can be generated as well.
# Writeback specifiers (%n) can be generated as well, and the written value is
# checked (against the reference implementation).
# One of the "specifiers" is "", which stands for a random-length literal string
# inside the format string (ie not a %-initiated specifier).
# The tests also include some checks against buffer/writeback overrun.
# Serious malfunctions of printf cannot be detected with certainy -- they may
# fail silently, fail with an appropriate message, produce a crash, etc.
# Only specifiers/modifiers relevant to NPF are handled. %a %A %p are supported,
# but they may turn out to be unusable because they have too much leeway for
# implementation-defined behavior. %a %A are only really predictable when the
# input value is +-0.0 .
# nan, inf, and subnormals are supported too; they can have the same problems of
# implementation-defined behavior, so they can be disabled.
# Among nans, the various mantissa payloads are (mostly) uniformly random.
#
# The terminology used here is the same as the standard one.
# The format string consists of literal characters and/or "conversion
# specifications". Each specification has this format:
#   %<flags><width><precision><length_modifier><conversion_specifier>
# All but the 

import math
import random
import struct

#random.seed(1) # for reproducible debugging

### PARAMETERS ###

out_file = "npf_fuzz_test.c"

npf_path = ""

# ref: None to use system printf
# include: all the code necessary to use such a printf (include, options, etc.)
ref_vsnprintf = None
code_for_ref_vsnprintf = None

npf_opts = {
    "NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS" : 1         ,
    "NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS"   : 1         ,
    "NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"       : 1         ,
    "NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS"       : 1         ,
    "NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS"       : 1         ,
    "NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS"      : 1         ,
    "NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS"   : 1         ,
    "NANOPRINTF_USE_ALT_FORM_FLAG"                 : 1         ,
    "NANOPRINTF_CONVERSION_BUFFER_SIZE"            : 512       ,
    "NANOPRINTF_CONVERSION_FLOAT_TYPE"             : "uint64_t",
}

num_tests = 10
max_num_specs = 10 # >= 1
can_repeat_flags = False
min_width = -100 # negative is only meaningful for "*"; literals will be limited to >0
max_width = 100
min_prec_i = -100 # negative is only meaningful for ".*"; literals will be limited to >=0
max_prec_i = 100
min_prec_f = -15 # negative is only meaningful for ".*"; literals will be limited to >=0
max_prec_f = 15
allow_nan = True
allow_inf = True
allow_subnormal = True
allow_opts_for_spec_percent = False # enable UB opts (flags, width, prec)
allow_opts_for_spec_c = False # enable UB opts (prec)
min_exp10_spec_f = -15 # None: allow any
max_exp10_spec_f = 15 # None: allow any
only_0_for_spec_a = True

# All the following tables list items with their weight (unnormalized probability)

specs_table = [
  #["a", 1], ["A", 1],
  #["b", 1], ["B", 1],
  ["c", 1],
  ["d", 1], ["i", 1],
  #["e", 1], ["E", 1],
  ["f", 1], ["F", 1],
  #["g", 1], ["G", 1],
  ["n", 1],
  ["o", 1],
  #["p", 1], # implementation-defined
  ["s", 1],
  ["u", 1],
  ["x", 1], ["X", 1],
  ["%", 1],
  ["" , 1],
]

# only for this table, probabilities are normalized (since flags can be cumulated,
# whereas in the other tables, items are mutually exclusive)
flags_table = [
  [" ", .2],
  ["+", .2],
  ["-", .2],
  ["0", .2],
  ["#", .2],
]

widths_table = [
  ["n", 1],
  ["*", 1],
  ["" , 1],
]

precs_table = [
  ["n", 1],
  ["*", 1],
  ["" , 1],
]

modifs_table_i = [
  ["hh" , 1],
  ["h"  , 1],
  [""   , 1],
  ["l"  , 1],
  ["ll" , 1],
  ["j"  , 1],
  ["z"  , 1],
  ["t"  , 1],
]

modifs_table_f = [
  ["L", 0.1],
  ["l", 1],
  ["" , 1],
]

### END OF PARAMETERS ###

####

# convert from [[v, prob], [v, prob], ...] to [[v, v, ...], [prob, prob, ...]]
def prepare_table(table):
    return [[x[0] for x in table], [x[1] for x in table]]


def replace_weight(table, key, weight):
    for i in range(len(table)):
        if table[i][0] == key:
            table[i][1] = weight


if npf_opts["NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"] != 1:
    replace_weight(specs_table, "a", 0)
    replace_weight(specs_table, "A", 0)
    replace_weight(specs_table, "e", 0)
    replace_weight(specs_table, "E", 0)
    replace_weight(specs_table, "f", 0)
    replace_weight(specs_table, "F", 0)
    replace_weight(specs_table, "g", 0)
    replace_weight(specs_table, "G", 0)
if npf_opts["NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS"] != 1:
    replace_weight(widths_table, "n", 0)
    replace_weight(widths_table, "*", 0)
    replace_weight(flags_table, "0", 0)
    replace_weight(flags_table, "-", 0)
if npf_opts["NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS"] != 1:
    replace_weight(precs_table, "n", 0)
    replace_weight(precs_table, "*", 0)
if npf_opts["NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS"] != 1:
    replace_weight(modifs_table_i, "ll", 0)
    replace_weight(modifs_table_i, "j", 0)
    replace_weight(modifs_table_i, "z", 0)
    replace_weight(modifs_table_i, "t", 0)
if npf_opts["NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS"] != 1:
    replace_weight(modifs_table_i, "hh", 0)
    replace_weight(modifs_table_i, "h", 0)
if npf_opts["NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS"] != 1:
    replace_weight(specs_table, "b", 0)
    replace_weight(specs_table, "B", 0)
if npf_opts["NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS"] != 1:
    replace_weight(specs_table, "n", 0)
if npf_opts["NANOPRINTF_USE_ALT_FORM_FLAG"] != 1:
    replace_weight(flags_table, "#", 0)

specs_table = prepare_table(specs_table)
flags_table = prepare_table(flags_table)
widths_table = prepare_table(widths_table)
precs_table = prepare_table(precs_table)
modifs_table_i = prepare_table(modifs_table_i)
modifs_table_f = prepare_table(modifs_table_f)


def bits_to_float(bits):
    bb = bytes([((bits >> (8 * i)) & 0xFF) for i in range(8)])
    return struct.unpack('d', bb)[0]


def escape_string(s):
    s = s.replace("\\", "\\\\")
    s = s.replace("\"", "\\\"")
    s = s.replace("?", "\\?") # avoid problems with trigraphs
    return s


def escape_fmt_string(s):
    s = s.replace("\\", "\\\\")
    s = s.replace("\"", "\\\"")
    s = s.replace("%", "%%")
    s = s.replace("?", "\\?") # avoid problems with trigraphs
    return s


def escape_char(s):
    s = s.replace("\\", "\\\\")
    s = s.replace("\'", "\\\'")
    return s


alphabet = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
alphabet_no_percent = ' !"#$&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'


def gen_random_string(n):
    s = random.choices(alphabet, k=n)
    s = "".join(s)
    return s


def gen_random_fmt_string(n):
    s = random.choices(alphabet_no_percent, k=n)
    s = "".join(s)
    return s


def gen_flags():
    f = []
    for item in zip(flags_table[0], flags_table[1]):
        while True:
            p = random.random()
            if p < item[1]:
                f += item[0]
                if can_repeat_flags:
                    continue
            break
    random.shuffle(f)
    f = "".join(f)
    return [f, []]


def gen_width():
    w = random.choices(widths_table[0], widths_table[1], k=1)[0]
    if w == "":
        return ["", []]
    elif w == "n":
        v = random.randint(max(1, min_width), max_width)
        return ["{}".format(v), []]
    elif w == "*":
        v = random.randint(min_width, max_width)
        return ["*", ["{}".format(v)]]
    else:
        raise Exception("Unknown width: " + w)


def gen_prec(min_p, max_p):
    p = random.choices(precs_table[0], precs_table[1], k=1)[0]
    if p == "":
        return ["", []]
    elif p == "n":
        v = random.randint(max(0, min_p), max_p)
        return [".{}".format(v), []]
    elif p == "*":
        v = random.randint(min_p, max_p)
        return [".*", ["{}".format(v)]]
    else:
        raise Exception("Unknown prec: " + p)


def gen_modif_i():
    m = random.choices(modifs_table_i[0], modifs_table_i[1], k=1)[0]
    return [m, []]


def gen_modif_f():
    m = random.choices(modifs_table_f[0], modifs_table_f[1], k=1)[0]
    return [m, []]


log2_10 = math.log2(10)
def trunc_n_log2_of_10(n):
    neg = (n < 0)
    v = (math.floor(log2_10 * abs(n)))
    if neg:
        v = -v-1
    else:
        v += 2
    return v


def gen_random_float(spec, modif):
    # always assume double is f64; and long double is treated the same
    # that's why we ignore 'modif'
    #
    # This routine is uniformly random in the bits, *not* in the float values.
    # That is, very small values are preferred, since they have a lot more
    # resolution. This is what we want though -- we do not care about the
    # actual values, we care about exercising the printf routine in many
    # different conditions (ie at all different scales of values).
    # The routine is only uniform within normal numbers. Zero/nan/inf have their
    # own probabilities (otherwise, 0.0 would be almost impossible to get).
    # Subnormals are uniform with the normals, but if they are not allowed, they
    # will be flushed to 0.
    # 0.0 is possible even if it is outside of [min_exp10_spec_f, max_exp10_spec_f]
    # The limit of [min_exp10_spec_f, max_exp10_spec_f] is not perfect. To simplify things,
    # we limit the binary exponent (and then tweak it), so that we get a range
    # that is guaranteed to be in the limits, but that is at times more limited
    # (of a fraction of a decade).
    
    sign = random.randint(0, 1) << 63
    if only_0_for_spec_a and spec in ["a", "A"]:
        return sign | 0
    
    mant = random.randint(0, 0x000FFFFFFFFFFFFF)
    min_exp2 = -1023
    max_exp2 = 1024
    if spec in ["f", "F"]:
        min_exp2 = max(min_exp2, trunc_n_log2_of_10(min_exp10_spec_f))
        max_exp2 = min(max_exp2, trunc_n_log2_of_10(max_exp10_spec_f))
        max_exp2 = max(min_exp2, max_exp2)
    while True:
        x = random.random()
        if x < 0.05: # 0
            return sign | 0
        x -= 0.05
        
        if x < 0.05: # nan
            if allow_nan:
                return sign | 0x7FF0000000000000 | (mant if mant != 0 else 1)
            else:
                continue
        x -= 0.05
        
        if x < 0.05: # inf
            if allow_inf:
                return sign | 0x7FF0000000000000
            else:
                continue
        x -= 0.05
        
        exp = random.randint(min_exp2, max_exp2) + 1023
        if exp == 0 and not allow_subnormal: # subnormal -> 0
            return sign | 0
        #normal
        return sign | (exp << 52) | mant


def emit_const_string():
    n = random.randint(1, 10)
    str = gen_random_fmt_string(n)
    str = escape_fmt_string(str)
    return [str, []]


def emit_c(s):
    v = gen_random_string(1)
    v = escape_char(v)
    fmt = "%"
    args = []
    
    res = gen_flags()
    fmt += res[0]
    args += res[1]
    
    res = gen_width()
    fmt += res[0]
    args += res[1]
    
    if allow_opts_for_spec_c:
        res = gen_prec(min_prec_i, max_prec_i)
        fmt += res[0]
        args += res[1]
    
    fmt += s
    args += ["'{}'".format(v)]
    return [fmt, args]


def emit_s(s):
    n = random.randint(0, 200)
    v = gen_random_string(n)
    v = escape_string(v)
    fmt = "%"
    args = []
    
    res = gen_flags()
    fmt += res[0]
    args += res[1]
    
    res = gen_width()
    fmt += res[0]
    args += res[1]
    
    res = gen_prec(min_prec_i, max_prec_i)
    fmt += res[0]
    args += res[1]
    
    fmt += s
    args += ["\"{}\"".format(v)]
    return [fmt, args]


def emit_f(s):
    fmt = "%"
    args = []
    
    res = gen_flags()
    fmt += res[0]
    args += res[1]
    
    res = gen_width()
    fmt += res[0]
    args += res[1]
    
    res = gen_prec(min_prec_f, max_prec_f)
    fmt += res[0]
    args += res[1]
    
    res = gen_modif_f()
    modif = res[0]
    fmt += res[0]
    args += res[1]
    
    # always assume double is f64; and long double is treated the same
    if modif == "L":
        v = gen_random_float(s, modif)
        fmt += s
        args += ["(long double)npf_u64_to_dbl(0x{:016X}) /* {:.17g} */".format(v, bits_to_float(v))]
    else:
        v = gen_random_float(s, modif)
        fmt += s
        args += ["npf_u64_to_dbl(0x{:016X}) /* {:.17g} */".format(v, bits_to_float(v))]
    return [fmt, args]


def emit_i(s):
    fmt = "%"
    args = []
    
    res = gen_flags()
    fmt += res[0]
    args += res[1]
    
    res = gen_width()
    fmt += res[0]
    args += res[1]
    
    res = gen_prec(min_prec_i, max_prec_i)
    fmt += res[0]
    args += res[1]
    
    res = gen_modif_i()
    modif = res[0]
    fmt += res[0]
    args += res[1]
    
    # always generate u64, then cast as needed
    v = random.randint(0, 0xFFFFFFFFFFFFFFFF)
    fmt += s
    if s in ["d", "i"]: # signed
        if modif in ["hh", "h", ""]:
            args += ["(int)(0x{:X})".format(v)]
        elif modif == "l":
            args += ["(long)(0x{:X})".format(v)]
        elif modif == "ll":
            args += ["(long long)(0x{:X})".format(v)]
        elif modif == "j":
            args += ["(intmax_t)(0x{:X})".format(v)]
        elif modif == "z":
            args += ["(size_t)(0x{:X})".format(v)]
        elif modif == "t":
            args += ["(ptrdiff_t)(0x{:X})".format(v)]
        else:
            raise Exception("Unknown modif: " + modif)
    else: # unsigned
        if modif in ["hh", "h", ""]:
            args += ["(unsigned)(0x{:X})".format(v)]
        elif modif == "l":
            args += ["(unsigned long)(0x{:X})".format(v)]
        elif modif == "ll":
            args += ["(unsigned long long)(0x{:X})".format(v)]
        elif modif == "j":
            args += ["(uintmax_t)(0x{:X})".format(v)]
        elif modif == "z":
            args += ["(size_t)(0x{:X})".format(v)]
        elif modif == "t":
            args += ["(ptrdiff_t)(0x{:X})".format(v)]
        else:
            raise Exception("Unknown modif: " + modif)
    return [fmt, args]


def emit_p(s):
    fmt = "%"
    args = []
    
    res = gen_flags()
    fmt += res[0]
    args += res[1]
    
    res = gen_width()
    fmt += res[0]
    args += res[1]
    
    res = gen_prec(min_prec_i, max_prec_i)
    fmt += res[0]
    args += res[1]
    
    # always generate u64, then cast as needed
    v = random.randint(0, 0xFFFFFFFFFFFFFFFF)
    fmt += s
    args += ["(void*)(0x{:016X})".format(v)]
    return [fmt, args]


def emit_n(s):
    fmt = "%"
    args = []
    
    res = gen_modif_i()
    modif = res[0]
    fmt += res[0]
    args += res[1]
    args += ["&p_data.n_" + modif + "i"] # %n is always signed
    fmt += s
    return [fmt, args]


def emit_percent(s):
    fmt = "%"
    args = []
    
    if allow_opts_for_spec_percent:
        res = gen_flags()
        fmt += res[0]
        args += res[1]
        
        res = gen_width()
        fmt += res[0]
        args += res[1]
        
        res = gen_prec(min_prec_i, max_prec_i)
        fmt += res[0]
        args += res[1]
    
    fmt += s
    return [fmt, args]


out = []
for t in range(num_tests):
    num_specs = random.randint(1, max_num_specs)
    specs = random.choices(specs_table[0], specs_table[1], k=num_specs)

    fmt = []
    args = []

    for s in specs:
        if s == "":
            (new_fmt, new_args) = emit_const_string()
        elif s == "c":
            (new_fmt, new_args) = emit_c(s)
        elif s == "s":
            (new_fmt, new_args) = emit_s(s)
        elif s in ["a", "A", "e", "E", "f", "F", "g", "G"]:
            (new_fmt, new_args) = emit_f(s)
        elif s in ["b", "B", "d", "i", "o", "u", "x", "X"]:
            (new_fmt, new_args) = emit_i(s)
        elif s == "p":
            (new_fmt, new_args) = emit_p(s)
        elif s == "n":
            (new_fmt, new_args) = emit_n(s)
        elif s == "%":
            (new_fmt, new_args) = emit_percent(s)
        else:
            raise Exception("Unknown spec: " + s)
        if new_fmt != "":
          fmt.append(new_fmt)
        if len(new_args) > 0:
          args.append(new_args)
    test_params = []
    test_params.append(["    ".join(["\"{}\"".format(f) for f in fmt])])
    test_params += args
    out.append(test_params)

opts = "\n".join(["#define {:50s}{}".format(k, npf_opts[k]) for k in npf_opts.keys()])


pre = r"""
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
"""
pre += opts
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
  }
#endif
}

int main(void) {
"""

post = r"""
  printf("done.\n");
}
"""

out = ["  test_print(" + x + ");" for x in [",\n    ".join([",    ".join(y) for y in z]) for z in out]]
out = "\n".join(out)
with open(out_file, 'w', encoding="utf-8") as f:
   f.write(pre)
   f.write(out)
   f.write(post)
print("Done.")