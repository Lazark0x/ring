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
#include "../../limbs/limbs.inl"

#if defined(__wasm32__)

// WebAssembly implementation of Montgomery multiplication
// This is the PRIMARY implementation for wasm32, NOT a fallback.
// Rust does NOT export bn_mul_mont for wasm32 - this C implementation is used directly.
// This ensures optimal WASM compilation without Rust fallback overhead.
void bn_mul_mont(BN_ULONG *r, const BN_ULONG *a, const BN_ULONG *b,
                 const BN_ULONG *n, const BN_ULONG n0_[BN_MONT_CTX_N0_LIMBS],
                 size_t num_limbs) {
  if (num_limbs == 0) {
    return;
  }

  BN_ULONG n0 = n0_[0];
  
  // Allocate temporary buffer for the product
  // Note: In practice, this should be stack-allocated with a reasonable limit
  BN_ULONG tmp[2 * 1024];  // Assuming max 1024 limbs (adjust based on MAX_LIMBS)
  
  if (num_limbs > 1024) {
    return;  // Safety check
  }

  // Step 1: Compute full multiplication a * b -> tmp
  // Initialize tmp to zero
  for (size_t i = 0; i < 2 * num_limbs; i++) {
    tmp[i] = 0;
  }

  // Multiply a * b using limbs_mul_add_limb
  for (size_t i = 0; i < num_limbs; i++) {
    tmp[num_limbs + i] = limbs_mul_add_limb(tmp + i, a, b[i], num_limbs);
  }

  // Step 2: Montgomery reduction - convert from Montgomery form
  // This implements the standard Montgomery reduction algorithm
  BN_ULONG carry = 0;
  for (size_t i = 0; i < num_limbs; i++) {
    BN_ULONG v = limbs_mul_add_limb(tmp + i, n, tmp[i] * n0, num_limbs);
    v += carry + tmp[i + num_limbs];
    carry |= (v != tmp[i + num_limbs]);
    carry &= (v <= tmp[i + num_limbs]);
    tmp[i + num_limbs] = v;
  }

  // Step 3: Final conditional subtraction
  // The result is in tmp[num_limbs..2*num_limbs-1]
  BN_ULONG *result = tmp + num_limbs;
  
  // Subtract n and select the result in constant time
  BN_ULONG v = limbs_sub(r, result, n, num_limbs) - carry;
  v = 0u - v;
  
  // Constant-time select: if underflow, use result; otherwise use r
  for (size_t i = 0; i < num_limbs; i++) {
    r[i] = constant_time_select_w(v, result[i], r[i]);
  }
}

#endif  // __wasm32__
