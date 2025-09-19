// Hidden shim for WebAssembly:
// Defines the prefixed name expected by C objects and delegates to the
// internal Rust implementation (__ring_bn_mul_mont_impl). Hidden visibility
// ensures the symbol does not appear in the WASM export section.
//
// Limb == usize in Rust; for wasm32 that is 32-bit.

#include <stdint.h>
#include <stddef.h>

// Internal Rust implementation (defined in src/arithmetic/ffi.rs for wasm32).
__attribute__((visibility("hidden")))
extern int32_t __ring_bn_mul_mont_impl(
    uintptr_t *r,
    const uintptr_t *a,
    const uintptr_t *b,
    const uintptr_t *n,
    uintptr_t n0,
    size_t n_limbs
);

// Public prefixed name expected by C code (hidden, so it won't be exported).
__attribute__((visibility("hidden")))
int32_t ring_core_0_17_14__bn_mul_mont(
    uintptr_t *r,
    const uintptr_t *a,
    const uintptr_t *b,
    const uintptr_t *n,
    uintptr_t n0,
    size_t n_limbs
) {
    return __ring_bn_mul_mont_impl(r, a, b, n, n0, n_limbs);
}
