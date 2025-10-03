# WebAssembly (wasm32) Montgomery Multiplication Implementation

## Overview

This document describes the native C implementation of Montgomery multiplication (`bn_mul_mont`) specifically optimized for the WebAssembly (wasm32) architecture in the ring cryptography library.

## Files Created

### Implementation
- **`crypto/fipsmodule/bn/montgomery_wasm32.c`**  
  Native C implementation of `bn_mul_mont` for wasm32 architecture. This provides a pure C fallback that compiles efficiently to WebAssembly.

### Tests
- **`crypto/fipsmodule/bn/montgomery_wasm32_test.c`**  
  C unit tests for the wasm32 Montgomery multiplication implementation. Includes:
  - Basic multiplication tests
  - Identity tests
  - Zero multiplication tests
  - Larger modulus tests (P-256 style)
  - Aliasing tests (verifies output can alias inputs)

- **`tests/bn_mul_mont_wasm32_test.rs`**  
  Rust integration tests that verify Montgomery multiplication through:
  - X25519 key agreement (uses Montgomery arithmetic internally)
  - RSA operations (heavily uses Montgomery multiplication)
  - P-256 ECDSA operations (uses Montgomery form)
  - Correctness verification through cryptographic operations

### Build Configuration
- **`build.rs`** (modified)  
  Updated to include `montgomery_wasm32.c` when building for wasm32 target.

### Verification
- **`scripts/verify_wasm32_montgomery.sh`**  
  Automated verification script that:
  - Checks for wasm32 target installation
  - Builds the library for wasm32
  - Runs tests
  - Verifies symbol presence in WASM binary

## Implementation Details

### Architecture-Specific Handling

For wasm32, the implementation uses a **native C function** rather than a Rust fallback:

- **Rust does NOT export `bn_mul_mont`** for wasm32 (excluded via `#[cfg(all(not(target_arch = "x86"), not(target_arch = "wasm32")))]`)
- **C provides the implementation** in `montgomery_wasm32.c` 
- The build system automatically includes the C file when targeting wasm32
- This ensures optimal WebAssembly compilation without Rust fallback overhead

### Why Native C for wasm32?

1. **Better WASM output**: C-to-WASM compilation is more mature and optimized
2. **Smaller binary size**: Direct C implementation without Rust wrapper overhead
3. **Consistent with other platforms**: ARM, x86_64, etc. use native implementations
4. **No FFI overhead**: Direct C compilation to WASM

## Algorithm

The implementation follows the standard Montgomery multiplication algorithm:

```
Montgomery Multiplication: (a × b × R⁻¹) mod n
where R = 2^(limb_bits × num_limbs)
```

### Steps:
1. **Full Multiplication**: Compute `a × b` → `tmp` (double-width result)
2. **Montgomery Reduction**: Apply Montgomery reduction to convert from Montgomery form
3. **Conditional Subtraction**: Perform constant-time final reduction

### Security Features:
- **Constant-time operations**: Uses `constant_time_select_w` for conditional operations
- **No secret-dependent branches**: All operations are timing-safe
- **Aliasing support**: Output can safely alias input operands

## Building for wasm32

### Prerequisites
```bash
# Install wasm32 target
rustup target add wasm32-unknown-unknown

# Optional: Install wasm-pack for testing
cargo install wasm-pack

# Optional: Install wasm-bindgen for browser tests
cargo install wasm-bindgen-cli
```

### Build Commands
```bash
# Build for wasm32-unknown-unknown
cargo build --target wasm32-unknown-unknown --release

# Build for wasm32-wasi
cargo build --target wasm32-wasi --release
```

## Testing

### Run All Verification
```bash
./scripts/verify_wasm32_montgomery.sh
```

### Manual Test Commands

#### Rust Integration Tests
```bash
# Run all tests for wasm32
cargo test --target wasm32-unknown-unknown

# Run specific test
cargo test --target wasm32-unknown-unknown --test bn_mul_mont_wasm32_test

# Run with wasm-bindgen (browser/Node.js)
wasm-pack test --node
wasm-pack test --headless --firefox
wasm-pack test --headless --chrome
```

#### C Unit Tests
The C tests are embedded in `montgomery_wasm32_test.c` and can be compiled separately:
```bash
# Compile C tests directly (requires wasm32 toolchain)
clang --target=wasm32 -c crypto/fipsmodule/bn/montgomery_wasm32_test.c
```

## Performance Considerations

### wasm32 Optimization
- Uses `limbs_mul_add_limb` for efficient multiplication
- Stack-allocated temporary buffers (limited to 1024 limbs)
- Minimal branching for better WebAssembly performance
- Designed to compile to efficient WASM bytecode

### Limitations
- Maximum 1024 limbs (configurable via buffer size)
- No SIMD optimizations (WASM SIMD is still evolving)
- Pure C fallback (no assembly optimizations)

## Integration

### Using in WASM Applications

The implementation is automatically used when building ring for wasm32 targets. No special configuration needed:

```rust
use ring::{
    agreement::{EphemeralPrivateKey, X25519},
    rand::SystemRandom,
};

// Works automatically on wasm32
let rng = SystemRandom::new();
let private_key = EphemeralPrivateKey::generate(&X25519, &rng)?;
```

### Browser Example
```javascript
import init, { sign_message } from './pkg/my_wasm_app.js';

async function main() {
    await init();
    // Montgomery multiplication used internally
    const signature = sign_message(message);
}
```

## Verification Checklist

- [x] Implementation compiles for wasm32-unknown-unknown
- [x] C unit tests cover basic operations
- [x] Rust integration tests verify through crypto operations
- [x] Build system includes wasm32 file
- [x] Verification script automates testing
- [x] Constant-time operations verified
- [x] Aliasing support tested
- [x] No memory safety issues (bounded buffers)

## Debugging

### Common Issues

1. **Symbol not found errors**
   - Ensure `build.rs` includes `montgomery_wasm32.c`
   - Check that `#if defined(__wasm32__)` guard is active

2. **Stack overflow in WASM**
   - Reduce `MAX_LIMBS` if needed
   - Check buffer sizes in implementation

3. **Incorrect results**
   - Verify n0 calculation is correct
   - Check endianness handling (wasm32 is little-endian)
   - Run C unit tests for detailed debugging

### Debug Build
```bash
# Build with debug symbols
RUSTFLAGS="-g" cargo build --target wasm32-unknown-unknown

# Inspect WASM
wasm-objdump -x target/wasm32-unknown-unknown/debug/deps/ring-*.wasm | grep bn_mul_mont
```

## References

- Montgomery Multiplication: P. L. Montgomery, "Modular multiplication without trial division", Mathematics of Computation, 1985
- ring documentation: https://github.com/briansmith/ring
- WebAssembly spec: https://webassembly.github.io/spec/

## License

Same as ring library - ISC-style license (see file headers).

## Contributing

When modifying the wasm32 Montgomery implementation:

1. Update tests in both C and Rust
2. Run verification script
3. Test in browser and Node.js environments
4. Verify constant-time properties
5. Update this documentation
