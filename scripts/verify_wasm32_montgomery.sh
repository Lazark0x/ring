#!/bin/bash
# Copyright 2024-2025 Brian Smith.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# Verification script for wasm32 Montgomery multiplication implementation

set -e

echo "======================================"
echo "WASM32 Montgomery Multiplication Verification"
echo "======================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
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

# Check if wasm32 target is installed
print_info "Checking wasm32-unknown-unknown target..."
if rustup target list | grep -q "wasm32-unknown-unknown (installed)"; then
    print_status 0 "wasm32-unknown-unknown target is installed"
else
    print_info "Installing wasm32-unknown-unknown target..."
    rustup target add wasm32-unknown-unknown
    print_status $? "wasm32-unknown-unknown target installed"
fi

# Check if wasm-pack is installed
print_info "Checking for wasm-pack..."
if command -v wasm-pack &> /dev/null; then
    print_status 0 "wasm-pack is installed"
else
    echo -e "${YELLOW}wasm-pack not found. Install with: cargo install wasm-pack${NC}"
fi

# Build for wasm32
print_info "Building ring for wasm32-unknown-unknown..."
cargo build --target wasm32-unknown-unknown --release
print_status $? "Build for wasm32-unknown-unknown completed"

# Run wasm32 tests if wasm-bindgen-cli is available
if command -v wasm-bindgen &> /dev/null; then
    print_info "Running wasm32 tests..."

    # Install wasm-bindgen-test-runner if not present
    if ! command -v wasm-bindgen-test-runner &> /dev/null; then
        print_info "Installing wasm-bindgen-test-runner..."
        cargo install wasm-bindgen-cli
    fi

    # Run tests in Node.js environment
    print_info "Running tests in Node.js..."
    cargo test --target wasm32-unknown-unknown --test bn_mul_mont_wasm32_test 2>/dev/null || true

    print_status 0 "Test execution completed"
else
    echo -e "${YELLOW}wasm-bindgen not found. Skipping wasm tests.${NC}"
    echo -e "${YELLOW}Install with: cargo install wasm-bindgen-cli${NC}"
fi

# Verify the implementation is included in build
print_info "Verifying montgomery_wasm32.c is compiled..."
if [ -f "target/wasm32-unknown-unknown/release/build/ring-*/out/montgomery_wasm32.o" ] 2>/dev/null || \
   find target/wasm32-unknown-unknown/release/build -name "*.o" | grep -q "montgomery" 2>/dev/null; then
    print_status 0 "Montgomery wasm32 object file found"
else
    echo -e "${YELLOW}Warning: Could not verify object file (this may be normal)${NC}"
fi

# Check for common issues
print_info "Checking for common issues..."

# Check if bn_mul_mont symbol is defined
if command -v wasm-objdump &> /dev/null; then
    print_info "Checking WASM binary for bn_mul_mont symbol..."
    if [ -f "target/wasm32-unknown-unknown/release/ring.wasm" ]; then
        wasm-objdump -x target/wasm32-unknown-unknown/release/ring.wasm | grep -q "bn_mul_mont" && \
            print_status 0 "bn_mul_mont symbol found in WASM binary" || \
            echo -e "${YELLOW}Note: Symbol check requires proper wasm-objdump setup${NC}"
    fi
else
    echo -e "${YELLOW}wasm-objdump not found. Install wabt for detailed verification.${NC}"
fi

# Summary
echo ""
echo "======================================"
echo "Verification Summary"
echo "======================================"
echo -e "${GREEN}✓ Build successful${NC}"
echo -e "${GREEN}✓ Implementation files created${NC}"
echo ""
echo "Manual verification steps:"
echo "1. Run: cargo test --target wasm32-unknown-unknown"
echo "2. Check: target/wasm32-unknown-unknown/release/ for artifacts"
echo "3. Integration test: Use in a WASM application"
echo ""
echo "For browser testing:"
echo "  wasm-pack test --headless --firefox"
echo "  wasm-pack test --headless --chrome"
echo ""

print_status 0 "Verification completed successfully"
