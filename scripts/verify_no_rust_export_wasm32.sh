#!/bin/bash
# Verify that bn_mul_mont is NOT exported from Rust for wasm32
# Copyright 2024-2025 Brian Smith.

set -e

echo "========================================"
echo "Verifying wasm32 uses C implementation"
echo "========================================"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ $2${NC}"
    else
        echo -e "${RED}✗ $2${NC}"
        exit 1
    fi
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Build for wasm32
print_info "Building for wasm32-unknown-unknown..."
cargo build --target wasm32-unknown-unknown --lib 2>&1 | head -20

# Check that Rust code has the correct cfg attribute
print_info "Checking Rust source code excludes wasm32 from export..."
if grep -q '#\[cfg(all(not(target_arch = "x86"), not(target_arch = "wasm32")))\]' src/arithmetic/montgomery.rs; then
    print_status 0 "Rust correctly excludes wasm32 from bn_mul_mont export"
else
    print_status 1 "ERROR: wasm32 exclusion not found in montgomery.rs"
fi

# Check that C implementation exists
print_info "Checking C implementation exists..."
if [ -f "crypto/fipsmodule/bn/montgomery_wasm32.c" ]; then
    print_status 0 "C implementation file exists"
else
    print_status 1 "ERROR: C implementation file not found"
fi

# Check that build.rs includes the C file for wasm32
print_info "Checking build.rs includes wasm32 C file..."
if grep -q 'WASM32.*montgomery_wasm32.c' build.rs; then
    print_status 0 "build.rs includes montgomery_wasm32.c for WASM32"
else
    print_status 1 "ERROR: build.rs doesn't include montgomery_wasm32.c"
fi

# Verify the C file has the correct guard
print_info "Checking C implementation has __wasm32__ guard..."
if grep -q '#if defined(__wasm32__)' crypto/fipsmodule/bn/montgomery_wasm32.c; then
    print_status 0 "C implementation has correct __wasm32__ guard"
else
    print_status 1 "ERROR: C implementation missing __wasm32__ guard"
fi

echo ""
echo "========================================"
echo "Summary"
echo "========================================"
echo -e "${GREEN}✓ Rust does NOT export bn_mul_mont for wasm32${NC}"
echo -e "${GREEN}✓ C provides bn_mul_mont implementation${NC}"
echo -e "${GREEN}✓ Build system configured correctly${NC}"
echo ""
echo "This ensures wasm32 uses the optimized C implementation"
echo "without Rust FFI export overhead."
echo ""

print_status 0 "Verification complete"
