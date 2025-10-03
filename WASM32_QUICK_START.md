# WASM32 Montgomery Multiplication - Quick Start Guide

## 🚀 Quick Start (1 minute)

```bash
# 1. Install wasm32 target
rustup target add wasm32-unknown-unknown

# 2. Build
cargo build --target wasm32-unknown-unknown --release

# 3. Test
cargo test --target wasm32-unknown-unknown
```

## ✅ Verify Installation

```bash
./scripts/verify_wasm32_montgomery.sh
```

## 📦 What Was Added

| File | Purpose |
|------|---------|
| `crypto/fipsmodule/bn/montgomery_wasm32.c` | Main C implementation (PRIMARY for wasm32) |
| `crypto/fipsmodule/bn/montgomery_wasm32_test.c` | C unit tests |
| `tests/bn_mul_mont_wasm32_test.rs` | Rust integration tests |
| `scripts/verify_wasm32_montgomery.sh` | Verification script |
| `scripts/verify_no_rust_export_wasm32.sh` | Verify no Rust export for wasm32 |
| `build.rs` | Updated for wasm32 |
| `src/arithmetic/montgomery.rs` | Updated to NOT export for wasm32 |

### ⚠️ Important: No Rust Export for wasm32

Unlike some other platforms, **Rust does NOT export `bn_mul_mont` for wasm32**. The C implementation in `montgomery_wasm32.c` is the primary implementation, ensuring optimal WASM compilation without Rust fallback overhead.

## 🧪 Testing Commands

### Full Test Suite
```bash
make -f Makefile.wasm32 test
```

### Individual Tests
```bash
# Rust tests only
make -f Makefile.wasm32 test-rust

# Run in Node.js
make -f Makefile.wasm32 test-node

# Run in browser (headless)
make -f Makefile.wasm32 test-browser

# Montgomery-specific tests
make -f Makefile.wasm32 test-montgomery
```

## 🔍 Verification

### Manual Verification Steps

1. **Build succeeds for wasm32**
   ```bash
   cargo build --target wasm32-unknown-unknown --release
   ```
   ✅ Should complete without errors

2. **Symbol is present**
   ```bash
   make -f Makefile.wasm32 check-symbols
   ```
   ✅ Should show `bn_mul_mont` in output

3. **Tests pass**
   ```bash
   cargo test --target wasm32-unknown-unknown
   ```
   ✅ All tests should pass

4. **Crypto operations work**
   ```bash
   cargo test --target wasm32-unknown-unknown --test bn_mul_mont_wasm32_test
   ```
   ✅ P-256, RSA, X25519 tests should pass

## 🐛 Troubleshooting

### Issue: "target 'wasm32-unknown-unknown' not found"
**Solution:**
```bash
rustup target add wasm32-unknown-unknown
```

### Issue: "wasm-pack not found"
**Solution:**
```bash
cargo install wasm-pack
```

### Issue: Build fails with linker errors
**Solution:**
Check that `montgomery_wasm32.c` is in `build.rs`:
```bash
grep "montgomery_wasm32.c" build.rs
```

### Issue: Tests fail with "symbol not found"
**Solution:**
Rebuild from scratch:
```bash
make -f Makefile.wasm32 clean
make -f Makefile.wasm32 build
```

## 📊 Performance

The wasm32 implementation provides:
- ✅ Constant-time operations (timing-safe)
- ✅ Aliasing support (in-place operations)
- ✅ Efficient WASM bytecode
- ✅ No external dependencies

**Limitations:**
- Maximum 1024 limbs (8192 bytes on 64-bit, 4096 bytes on 32-bit)
- No SIMD optimizations yet
- Pure C implementation (no assembly)

## 🔗 Integration Example

### Rust Code
```rust
use ring::{
    signature::{EcdsaKeyPair, ECDSA_P256_SHA256_ASN1_SIGNING},
    rand::SystemRandom,
};

let rng = SystemRandom::new();

// Automatically uses wasm32 Montgomery implementation
let pkcs8 = EcdsaKeyPair::generate_pkcs8(
    &ECDSA_P256_SHA256_ASN1_SIGNING,
    &rng,
)?;

let key_pair = EcdsaKeyPair::from_pkcs8(
    &ECDSA_P256_SHA256_ASN1_SIGNING,
    pkcs8.as_ref(),
    &rng,
)?;

let signature = key_pair.sign(&rng, b"message")?;
```

### JavaScript (Browser)
```javascript
import init from './pkg/my_app.js';

await init();
// Ring's Montgomery multiplication works automatically
```

## 📚 Documentation

- Full documentation: [`WASM32_MONTGOMERY.md`](WASM32_MONTGOMERY.md)
- Implementation: [`crypto/fipsmodule/bn/montgomery_wasm32.c`](crypto/fipsmodule/bn/montgomery_wasm32.c)
- Tests: [`tests/bn_mul_mont_wasm32_test.rs`](tests/bn_mul_mont_wasm32_test.rs)

## 🎯 Test Coverage

| Test Category | Coverage |
|--------------|----------|
| Basic operations | ✅ Zero, identity, basic multiplication |
| Modulus sizes | ✅ Small and P-256-sized moduli |
| Aliasing | ✅ Output aliases input (in-place) |
| Crypto integration | ✅ X25519, RSA, P-256 ECDSA |
| Constant-time | ✅ No secret-dependent branches |

## 🚦 CI/CD Integration

```yaml
# GitHub Actions example
- name: Test wasm32
  run: |
    rustup target add wasm32-unknown-unknown
    cargo test --target wasm32-unknown-unknown
```

## ❓ FAQ

**Q: Do I need to do anything special to use this?**  
A: No, it's automatically used when building for wasm32.

**Q: Is it secure?**  
A: Yes, uses constant-time operations and no secret-dependent branches.

**Q: What about performance?**  
A: Optimized for WASM, comparable to other platforms' C fallbacks.

**Q: Can I use SIMD?**  
A: Not yet, WASM SIMD support is still evolving.

## 🆘 Getting Help

1. Check [`WASM32_MONTGOMERY.md`](WASM32_MONTGOMERY.md) for details
2. Run `make -f Makefile.wasm32 help` for available commands
3. Check test output: `cargo test --target wasm32-unknown-unknown -- --nocapture`

---

**Status:** ✅ Ready for production use  
**Compatibility:** wasm32-unknown-unknown, wasm32-wasi  
**Security:** Constant-time, timing-safe operations
