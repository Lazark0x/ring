#include "internal.h"
#include "../../internal.h"

#include "../../limbs/limbs.h"
#include "../../limbs/limbs.inl"

// Internal Rust implementation (defined in src/arithmetic/ffi.rs for wasm32).
__attribute__((visibility("hidden")))
extern void __ring_bn_mul_mont_impl(BN_ULONG *rp, const BN_ULONG *ap, const BN_ULONG *bp,
                                    const BN_ULONG *np, const BN_ULONG *n0, size_t num);

// Public prefixed name expected by C code (hidden, so it won't be exported).
__attribute__((visibility("hidden")))
void bn_mul_mont(BN_ULONG *rp, const BN_ULONG *ap, const BN_ULONG *bp,
                 const BN_ULONG *np, const BN_ULONG *n0, size_t num) {
    __ring_bn_mul_mont_impl(rp, ap, bp, np, n0, num);
};
