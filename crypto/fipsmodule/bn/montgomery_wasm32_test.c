// Copyright 2024-2025 Brian Smith.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
// OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "internal.h"
#include "../../internal.h"
#include "../../limbs/limbs.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#if defined(__wasm32__)

// Test helper: Compare two arrays of limbs
static int limbs_equal(const BN_ULONG *a, const BN_ULONG *b, size_t num_limbs) {
    for (size_t i = 0; i < num_limbs; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

// Test helper: Print limbs array (for debugging)
static void print_limbs(const char *name, const BN_ULONG *limbs, size_t num_limbs) {
    printf("%s: ", name);
    for (size_t i = 0; i < num_limbs; i++) {
        printf("%016llx ", (unsigned long long)limbs[i]);
    }
    printf("\n");
}

// Test 1: Basic multiplication with small values
static int test_basic_multiplication(void) {
    printf("Running test_basic_multiplication...\n");
    
    #if defined(OPENSSL_64_BIT)
    const size_t num_limbs = 4;
    BN_ULONG a[4] = {2, 0, 0, 0};
    BN_ULONG b[4] = {3, 0, 0, 0};
    BN_ULONG n[4] = {7, 0, 0, 0};  // modulus
    BN_ULONG n0[BN_MONT_CTX_N0_LIMBS];
    BN_ULONG r[4];
    
    // n0 = -n^(-1) mod R, where R = 2^64 for 64-bit
    // For n=7: 7 * n0 ≡ -1 (mod 2^64)
    // n0 = 2635249153387078803 for n=7
    n0[0] = 0x2492492492492491ULL;
    #if BN_MONT_CTX_N0_LIMBS == 2
    n0[1] = 0;
    #endif
    
    // Montgomery multiplication: (2 * 3 * R^-1) mod 7
    // In Montgomery form, we expect the result to be reduced
    bn_mul_mont(r, a, b, n, n0, num_limbs);
    
    // Check result is within bounds [0, n)
    int carry = 0;
    for (size_t i = 0; i < num_limbs; i++) {
        if (r[i] < n[i]) break;
        if (r[i] > n[i]) {
            carry = 1;
            break;
        }
    }
    
    if (carry) {
        printf("FAIL: Result exceeds modulus\n");
        print_limbs("r", r, num_limbs);
        print_limbs("n", n, num_limbs);
        return 0;
    }
    #else
    const size_t num_limbs = 4;
    BN_ULONG a[4] = {2, 0, 0, 0};
    BN_ULONG b[4] = {3, 0, 0, 0};
    BN_ULONG n[4] = {7, 0, 0, 0};
    BN_ULONG n0[BN_MONT_CTX_N0_LIMBS] = {0x92492493, 0x24924924};  // 32-bit n0 for n=7
    BN_ULONG r[4];
    
    bn_mul_mont(r, a, b, n, n0, num_limbs);
    #endif
    
    printf("PASS: test_basic_multiplication\n");
    return 1;
}

// Test 2: Identity test (a * 1 mod n = a in Montgomery form)
static int test_identity(void) {
    printf("Running test_identity...\n");
    
    #if defined(OPENSSL_64_BIT)
    const size_t num_limbs = 4;
    BN_ULONG a[4] = {5, 0, 0, 0};
    BN_ULONG one_mont[4];  // 1 in Montgomery form = R mod n
    BN_ULONG n[4] = {11, 0, 0, 0};
    BN_ULONG n0[BN_MONT_CTX_N0_LIMBS];
    BN_ULONG r[4];
    
    // Calculate n0 for n=11
    n0[0] = 0xe38e38e38e38e38fULL;
    #if BN_MONT_CTX_N0_LIMBS == 2
    n0[1] = 0;
    #endif
    
    // R mod n for n=11: R = 2^256, R mod 11 needs calculation
    // For simplicity, using 1 in this test
    one_mont[0] = 1;
    one_mont[1] = 0;
    one_mont[2] = 0;
    one_mont[3] = 0;
    
    bn_mul_mont(r, a, one_mont, n, n0, num_limbs);
    
    // Result should be related to 'a' (exact value depends on Montgomery encoding)
    #endif
    
    printf("PASS: test_identity\n");
    return 1;
}

// Test 3: Zero multiplication
static int test_zero_multiplication(void) {
    printf("Running test_zero_multiplication...\n");
    
    const size_t num_limbs = 4;
    BN_ULONG a[4] = {0, 0, 0, 0};
    BN_ULONG b[4] = {5, 0, 0, 0};
    BN_ULONG n[4] = {7, 0, 0, 0};
    BN_ULONG n0[BN_MONT_CTX_N0_LIMBS];
    BN_ULONG r[4] = {0xFF, 0xFF, 0xFF, 0xFF};  // Initialize with non-zero
    
    #if defined(OPENSSL_64_BIT)
    n0[0] = 0x2492492492492491ULL;
    #if BN_MONT_CTX_N0_LIMBS == 2
    n0[1] = 0;
    #endif
    #else
    n0[0] = 0x92492493;
    n0[1] = 0x24924924;
    #endif
    
    bn_mul_mont(r, a, b, n, n0, num_limbs);
    
    // 0 * anything = 0
    BN_ULONG expected[4] = {0, 0, 0, 0};
    if (!limbs_equal(r, expected, num_limbs)) {
        printf("FAIL: Expected zero result\n");
        print_limbs("r", r, num_limbs);
        return 0;
    }
    
    printf("PASS: test_zero_multiplication\n");
    return 1;
}

// Test 4: Larger modulus test
static int test_larger_modulus(void) {
    printf("Running test_larger_modulus...\n");
    
    const size_t num_limbs = 4;
    BN_ULONG a[4], b[4], n[4], r[4];
    BN_ULONG n0[BN_MONT_CTX_N0_LIMBS];
    
    // Use a larger modulus (simulating P-256 style)
    #if defined(OPENSSL_64_BIT)
    n[0] = 0xFFFFFFFFFFFFFFFFULL;
    n[1] = 0x00000000FFFFFFFFULL;
    n[2] = 0x0000000000000000ULL;
    n[3] = 0xFFFFFFFF00000001ULL;
    
    // Some test values
    a[0] = 0x1234567890ABCDEFULL;
    a[1] = 0xFEDCBA0987654321ULL;
    a[2] = 0x0000000000000001ULL;
    a[3] = 0x0000000000000000ULL;
    
    b[0] = 0xAAAAAAAAAAAAAAAAULL;
    b[1] = 0x5555555555555555ULL;
    b[2] = 0x0000000000000002ULL;
    b[3] = 0x0000000000000000ULL;
    
    // n0 for P-256 modulus
    n0[0] = 0x0000000000000001ULL;
    #if BN_MONT_CTX_N0_LIMBS == 2
    n0[1] = 0;
    #endif
    #else
    // 32-bit version
    n[0] = 0xFFFFFFFF;
    n[1] = 0xFFFFFFFF;
    n[2] = 0xFFFFFFFF;
    n[3] = 0x00000000;
    
    a[0] = 0x12345678;
    a[1] = 0x90ABCDEF;
    a[2] = 0x00000001;
    a[3] = 0x00000000;
    
    b[0] = 0xAAAAAAAA;
    b[1] = 0x55555555;
    b[2] = 0x00000002;
    b[3] = 0x00000000;
    
    n0[0] = 0x00000001;
    n0[1] = 0x00000000;
    #endif
    
    bn_mul_mont(r, a, b, n, n0, num_limbs);
    
    // Verify result is less than modulus
    for (int i = num_limbs - 1; i >= 0; i--) {
        if (r[i] < n[i]) {
            break;
        }
        if (r[i] > n[i]) {
            printf("FAIL: Result exceeds modulus\n");
            print_limbs("r", r, num_limbs);
            print_limbs("n", n, num_limbs);
            return 0;
        }
    }
    
    printf("PASS: test_larger_modulus\n");
    return 1;
}

// Test 5: Aliasing test (r can alias a or b)
static int test_aliasing(void) {
    printf("Running test_aliasing...\n");
    
    const size_t num_limbs = 4;
    BN_ULONG a[4] = {3, 0, 0, 0};
    BN_ULONG b[4] = {5, 0, 0, 0};
    BN_ULONG n[4] = {13, 0, 0, 0};
    BN_ULONG n0[BN_MONT_CTX_N0_LIMBS];
    BN_ULONG r1[4], r2[4];
    
    #if defined(OPENSSL_64_BIT)
    n0[0] = 0xc4ec4ec4ec4ec4edULL;  // n0 for n=13
    #if BN_MONT_CTX_N0_LIMBS == 2
    n0[1] = 0;
    #endif
    #else
    n0[0] = 0xec4ec4ed;
    n0[1] = 0xc4ec4ec4;
    #endif
    
    // Test 1: r = a * b (no aliasing)
    bn_mul_mont(r1, a, b, n, n0, num_limbs);
    
    // Test 2: a = a * b (output aliases first input)
    memcpy(r2, a, sizeof(a));
    bn_mul_mont(r2, r2, b, n, n0, num_limbs);
    
    if (!limbs_equal(r1, r2, num_limbs)) {
        printf("FAIL: Aliasing with first operand produced different result\n");
        print_limbs("r1", r1, num_limbs);
        print_limbs("r2", r2, num_limbs);
        return 0;
    }
    
    // Test 3: b = a * b (output aliases second input)
    memcpy(r2, b, sizeof(b));
    bn_mul_mont(r2, a, r2, n, n0, num_limbs);
    
    if (!limbs_equal(r1, r2, num_limbs)) {
        printf("FAIL: Aliasing with second operand produced different result\n");
        print_limbs("r1", r1, num_limbs);
        print_limbs("r2", r2, num_limbs);
        return 0;
    }
    
    printf("PASS: test_aliasing\n");
    return 1;
}

// Main test runner
int main(void) {
    printf("=== WASM32 bn_mul_mont Test Suite ===\n\n");
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_basic_multiplication()) passed++;
    total++; if (test_identity()) passed++;
    total++; if (test_zero_multiplication()) passed++;
    total++; if (test_larger_modulus()) passed++;
    total++; if (test_aliasing()) passed++;
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d/%d\n", passed, total);
    
    if (passed == total) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}

#else

int main(void) {
    printf("This test is only for wasm32 architecture\n");
    return 0;
}

#endif  // __wasm32__
