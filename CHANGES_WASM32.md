# Changes Summary: WASM32 Montgomery Multiplication

## What Changed

### Rust Code Changes

**File: `src/arithmetic/montgomery.rs`**

Changed the export condition for `bn_mul_mont` to **exclude wasm32**:

```rust
// Before: exported for all non-x86 platforms
#[cfg(not(target_arch = "x86"))]
prefixed_export! {
    unsafe extern "C" fn bn_mul_mont(...) { ... }
}

// After: excludes wasm32 (uses C implementation instead)
#[cfg(all(not(target_arch = "x86"), not(target_arch = "wasm32")))]
prefixed_export! {
    unsafe extern "C" fn bn_mul_mont(...) { ... }
}
```

**Why?** For wasm32, we use a native C implementation instead of exporting from Rust, which results in:
- Better WASM compilation
- Smaller binary size  
- No Rust-to-C FFI overhead
- Consistent with platform-specific optimizations (x86_64, ARM, etc.)

### C Implementation

**File: `crypto/fipsmodule/bn/montgomery_wasm32.c`**

Added a complete Montgomery multiplication implementation specifically for wasm32:

```c
#if defined(__wasm32__)
void bn_mul_mont(BN_ULONG *r, const BN_ULONG *a, const BN_ULONG *b,
                 const BN_ULONG *n, const BN_ULONG n0_[BN_MONT_CTX_N0_LIMBS],
                 size_t num_limbs) {
    // Full Montgomery multiplication algorithm
    // - Constant-time operations
    // - Aliasing support
    // - Optimized for WASM
}
#endif
```

### Build System

**File: `build.rs`**

Added wasm32 C file to compilation:

```rust
(&[WASM32], "crypto/fipsmodule/bn/montgomery_wasm32.c"),
```

## Architecture Overview

### Before (Other Platforms)

```
Platform          | Implementation Source
------------------|----------------------
x86_64           | Assembly + C
ARM/AARCH64      | Assembly + C  
Other (non-x86)  | Rust fallback → exports bn_mul_mont
```

### After (Including wasm32)

```
Platform          | Implementation Source
------------------|----------------------
x86_64           | Assembly + C
ARM/AARCH64      | Assembly + C
wasm32           | Pure C (montgomery_wasm32.c) ← NEW
Other (non-x86)  | Rust fallback → exports bn_mul_mont
```

## Key Points

1. **wasm32 is special**: Unlike other fallback platforms, it gets a dedicated C implementation
2. **No Rust export**: The `#[cfg(all(not(target_arch = "x86"), not(target_arch = "wasm32")))]` guard prevents Rust from exporting `bn_mul_mont` for wasm32
3. **C provides the function**: The linker uses the C version from `montgomery_wasm32.c`
4. **Zero overhead**: Direct C-to-WASM compilation, no Rust wrapper

## Verification

Run this to verify the changes:

```bash
./scripts/verify_no_rust_export_wasm32.sh
```

This checks:
- ✅ Rust has the correct `#[cfg]` attribute excluding wasm32
- ✅ C implementation exists with `__wasm32__` guard
- ✅ Build system includes the C file
- ✅ No Rust export for wasm32

## Impact

### Positive
- ✅ Better WASM performance
- ✅ Smaller binary size
- ✅ More maintainable (dedicated implementation)
- ✅ Consistent with other platforms' approach

### Neutral
- No API changes
- Fully backward compatible
- Transparent to users

## Files Modified

1. `src/arithmetic/montgomery.rs` - Added wasm32 exclusion
2. `crypto/fipsmodule/bn/montgomery_wasm32.c` - Primary implementation
3. `build.rs` - Added wasm32 C file to build

## Testing

All existing tests pass. Additionally:
- C unit tests in `montgomery_wasm32_test.c`
- Rust integration tests in `tests/bn_mul_mont_wasm32_test.rs`
- Verification scripts confirm correct behavior

## Migration Guide

**For users:** No action needed. The change is transparent.

**For developers:**
- If modifying Montgomery multiplication, remember wasm32 uses C, not Rust
- Tests must cover both C and Rust paths
- Changes to `montgomery.rs` don't affect wasm32 directly
