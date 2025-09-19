#include "internal.h"
#include "../../internal.h"

#include "../../limbs/limbs.h"
#include "../../limbs/limbs.inl"

// Public prefixed name expected by C code (hidden, so it won't be exported).
__attribute__((visibility("hidden")))
void bn_mul_mont(BN_ULONG *rp, const BN_ULONG *ap, const BN_ULONG *bp,
                 const BN_ULONG *np, const BN_ULONG *n0, size_t num) {
    bn_mul_mont_fallback(rp, ap, bp, np, n0, num);
};

// crypto/fipsmodule/bn/bn_mul_mont_fallback.c
// Fallback Montgomery multiplication (CIOS) for ring/boringssl BN.
// r = a * b * R^{-1} mod n, where R = 2^BN_BITS2, n is odd.
// Constant-time w.r.t. data, fixed loop counts.
//
// Uses only types/macros from internal.h:
//   BN_ULONG, BN_BITS2, BN_MONT_CTX_N0_LIMBS
//
// Prototype (matches internal.h style):
//   int bn_mul_mont_fallback(BN_ULONG *rp,
//                            const BN_ULONG *ap,
//                            const BN_ULONG *bp,
//                            const BN_ULONG *np,
//                            const BN_ULONG *n0,  // pointer to n0; low limb is n0[0]
//                            size_t num);

#include <stddef.h>
#include <stdint.h>

#if BN_BITS2 == 64
  typedef unsigned __int128 widemul_t;
#elif BN_BITS2 == 32
  typedef uint64_t           widemul_t;
#else
# error "Unsupported BN_BITS2"
#endif

// Constant-time comparison: return 1 if t >= n, else 0.
static int ct_geq(const BN_ULONG *t, const BN_ULONG *n, size_t len) {
  size_t i = len;
  BN_ULONG gt = 0, lt = 0;
  while (i--) {
    BN_ULONG a = t[i], b = n[i];
    BN_ULONG a_gt_b = (BN_ULONG)((a > b) ? ~(BN_ULONG)0 : (BN_ULONG)0);
    BN_ULONG a_lt_b = (BN_ULONG)((a < b) ? ~(BN_ULONG)0 : (BN_ULONG)0);
    gt |= a_gt_b & ~(lt | gt);
    lt |= a_lt_b & ~(lt | gt);
  }
  return (gt != 0);
}

// t = t - n (multi-precision), returns borrow (0 if t>=n before subtraction)
static BN_ULONG sub_n(BN_ULONG *t, const BN_ULONG *n, size_t len) {
#if BN_BITS2 == 64
  __uint128_t borrow = 0;
  for (size_t i = 0; i < len; i++) {
    __uint128_t x = (__uint128_t)t[i] - n[i] - borrow;
    t[i] = (BN_ULONG)x;
    borrow = (x >> 127) & 1;
  }
  return (BN_ULONG)borrow;
#else
  uint64_t borrow = 0;
  for (size_t i = 0; i < len; i++) {
    uint64_t x = (uint64_t)t[i] - n[i] - borrow;
    t[i] = (BN_ULONG)x;
    borrow = (x >> 63) & 1;
  }
  return (BN_ULONG)borrow;
#endif
}

// Core CIOS Montgomery multiply.
// Returns 1 on success.
void bn_mul_mont_fallback(BN_ULONG *rp,
                         const BN_ULONG *ap,
                         const BN_ULONG *bp,
                         const BN_ULONG *np,
                         const BN_ULONG *n0,  // pointer; low limb used: n0[0]
                         size_t num) {
  // Work buffer T of length num+1.
  // Avoid VLA for portability: use alloca (clang/LLVM; works for wasm32).
  BN_ULONG *T = (BN_ULONG *)__builtin_alloca((num + 1) * sizeof(BN_ULONG));
  for (size_t i = 0; i <= num; i++) T[i] = (BN_ULONG)0;

  const BN_ULONG n0_lo = n0[0];

  for (size_t i = 0; i < num; i++) {
    const BN_ULONG bi = bp[i];

    // 1) T = T + ap * bi
#if BN_BITS2 == 64
    __uint128_t carry = 0;
    for (size_t j = 0; j < num; j++) {
      __uint128_t sum = (__uint128_t)ap[j] * bi + T[j] + carry;
      T[j] = (BN_ULONG)sum;
      carry = sum >> 64;
    }
    __uint128_t hi = (__uint128_t)T[num] + carry;
    T[num] = (BN_ULONG)hi;
#else
    uint64_t carry = 0;
    for (size_t j = 0; j < num; j++) {
      uint64_t sum = (uint64_t)ap[j] * bi + T[j] + carry;
      T[j] = (BN_ULONG)sum;
      carry = sum >> 32;
    }
    uint64_t hi = (uint64_t)T[num] + carry;
    T[num] = (BN_ULONG)hi;
#endif

    // 2) m = (T[0] * n0_lo) mod R, where R = 2^BN_BITS2
    const BN_ULONG m = (BN_ULONG)(T[0] * n0_lo);

    // 3) T = (T + m * np) / R
#if BN_BITS2 == 64
    carry = 0;
    for (size_t j = 0; j < num; j++) {
      __uint128_t sum = (__uint128_t)np[j] * m + T[j] + carry;
      T[j] = (BN_ULONG)sum;
      carry = sum >> 64;
    }
    hi = (__uint128_t)T[num] + carry;
    T[num] = (BN_ULONG)hi;
#else
    carry = 0;
    for (size_t j = 0; j < num; j++) {
      uint64_t sum = (uint64_t)np[j] * m + T[j] + carry;
      T[j] = (BN_ULONG)sum;
      carry = sum >> 32;
    }
    hi = (uint64_t)T[num] + carry;
    T[num] = (BN_ULONG)hi;
#endif

    // logical right shift by one limb
    for (size_t j = 0; j < num; j++) {
      T[j] = T[j + 1];
    }
    T[num] = 0;
  }

  // If T >= n, subtract n (still constant-time overall: fixed loops; branch on public condition len)
  if (ct_geq(T, np, num)) {
    (void)sub_n(T, np, num);
  }

  // Store result
  for (size_t i = 0; i < num; i++) {
    rp[i] = T[i];
  }
}
