#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <string.h>

static int npf_test_pass_count;
static int npf_test_fail_count;
static char npf_test_buf[512];
static char npf_test_sys_buf[512];

static void npf_test_null_putc(int c, void *ctx) { (void)c; (void)ctx; }

/* fmt is captured inside __VA_ARGS__ to avoid ##__VA_ARGS__ (a GNU extension
   unsupported by MSVC). __VA_ARGS__ always has at least one element (the format
   string), so no trailing-comma issue arises. */

#define NPF_TEST(expected, ...) do { \
    npf_snprintf(npf_test_buf, sizeof(npf_test_buf), __VA_ARGS__); \
    if (strcmp(npf_test_buf, expected) != 0) { \
        snprintf(npf_test_sys_buf, sizeof(npf_test_sys_buf), __VA_ARGS__); \
        fprintf(stderr, "FAIL [%s:%d]: expected=\"%s\" got=\"%s\" sys=\"%s\"\n", \
                __FILE__, __LINE__, expected, npf_test_buf, npf_test_sys_buf); \
        ++npf_test_fail_count; \
    } else { ++npf_test_pass_count; } \
} while(0)

#define NPF_TEST_DYN(expected_expr, ...) do { \
    const char *npf_test_dyn_exp_ = (expected_expr); \
    npf_snprintf(npf_test_buf, sizeof(npf_test_buf), __VA_ARGS__); \
    if (strcmp(npf_test_buf, npf_test_dyn_exp_) != 0) { \
        snprintf(npf_test_sys_buf, sizeof(npf_test_sys_buf), __VA_ARGS__); \
        fprintf(stderr, "FAIL [%s:%d]: expected=\"%s\" got=\"%s\" sys=\"%s\"\n", \
                __FILE__, __LINE__, npf_test_dyn_exp_, npf_test_buf, npf_test_sys_buf); \
        ++npf_test_fail_count; \
    } else { ++npf_test_pass_count; } \
} while(0)

#define NPF_TEST_SYS(...) do { \
    snprintf(npf_test_sys_buf, sizeof(npf_test_sys_buf), __VA_ARGS__); \
    npf_snprintf(npf_test_buf, sizeof(npf_test_buf), __VA_ARGS__); \
    if (strcmp(npf_test_buf, npf_test_sys_buf) != 0) { \
        fprintf(stderr, "FAIL [%s:%d]: sys=\"%s\" got=\"%s\"\n", \
                __FILE__, __LINE__, npf_test_sys_buf, npf_test_buf); \
        ++npf_test_fail_count; \
    } else { ++npf_test_pass_count; } \
} while(0)

#define NPF_TEST_RET(expected_ret, ...) do { \
    int npf_test_ret_ = npf_snprintf(npf_test_buf, sizeof(npf_test_buf), __VA_ARGS__); \
    if (npf_test_ret_ != (expected_ret)) { \
        fprintf(stderr, "FAIL [%s:%d]: expected_ret=%d got_ret=%d\n", \
                __FILE__, __LINE__, (expected_ret), npf_test_ret_); \
        ++npf_test_fail_count; \
    } else { ++npf_test_pass_count; } \
} while(0)

#define NPF_TEST_WB(expected_val, actual_val) do { \
    if ((expected_val) != (actual_val)) { \
        fprintf(stderr, "FAIL [%s:%d]: writeback expected=%d got=%d\n", \
                __FILE__, __LINE__, (int)(expected_val), (int)(actual_val)); \
        ++npf_test_fail_count; \
    } else { ++npf_test_pass_count; } \
} while(0)

#endif /* TEST_HARNESS_H */
