#!/bin/bash

# File naming conventions:
#   test_*.alo  - Tests that should compile successfully
#   error_*.alo - Tests that should fail compilation (expected errors)
#   skip_*.alo  - Tests to skip (work in progress, etc.)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPILER="$SCRIPT_DIR/build/aloha"
EXAMPLES_DIR="$SCRIPT_DIR/tests/programs"
TEMP_DIR=$(mktemp -d)

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "================================================"
echo "  Aloha Test Runner"
echo "================================================"
echo "Temporary directory: $TEMP_DIR"
echo ""

cleanup() {
    echo ""
    echo "Cleaning up temporary directory..."
    rm -rf "$TEMP_DIR"
    echo "Done!"
}

# Register cleanup on exit
trap cleanup EXIT

if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler not found at $COMPILER${NC}"
    echo "Please run: cmake --build build"
    exit 1
fi

# Copy all .alo files from subdirectories
find "$EXAMPLES_DIR" -name "*.alo" -exec cp {} "$TEMP_DIR/" \;

if [ -f "$SCRIPT_DIR/stdlib.o" ]; then
    cp "$SCRIPT_DIR/stdlib.o" "$TEMP_DIR/"
fi

cd "$TEMP_DIR"

total=0
passed=0
failed=0
skipped=0

for file in *.alo; do
    if [[ "$file" == skip_* ]]; then
        echo -e "${YELLOW}⊘ Skipping $file (marked as skip)${NC}"
        skipped=$((skipped + 1))
        continue
    fi

    total=$((total + 1))

    if [[ "$file" == error_* ]]; then
        echo -n "Testing $file (expect error)... "

        if compile_output=$("$COMPILER" "$file" 2>&1); then
            # Compilation succeeded when it should have failed
            echo -e "${RED}✗ UNEXPECTED SUCCESS${NC}"
            echo "  Expected compilation to fail but it succeeded"
            failed=$((failed + 1))
        else
            # Compilation failed as expected
            echo -e "${BLUE}✓ FAIL AS EXPECTED${NC}"
            passed=$((passed + 1))
        fi
        continue
    fi

    echo -n "Testing $file... "

    if ! compile_output=$("$COMPILER" "$file" 2>&1); then
        echo -e "${RED}✗ COMPILATION FAILED${NC}"
        echo "$compile_output" | grep -E "Error|error|Expected" | head -3
        failed=$((failed + 1))
        continue
    fi

    if ! echo "$compile_output" | grep -q "Linking successful"; then
        echo -e "${RED}✗ LINKING FAILED${NC}"
        echo "$compile_output" | grep -E "error" | head -2
        failed=$((failed + 1))
        continue
    fi

    echo -e "${GREEN}✓ PASS${NC}"
    passed=$((passed + 1))
done

echo ""
echo "================================================"
echo "  Test Summary"
echo "================================================"
echo -e "Total:   $total"
echo -e "${GREEN}Passed:  $passed${NC}"
if [ $failed -gt 0 ]; then
    echo -e "${RED}Failed:  $failed${NC}"
fi
if [ $skipped -gt 0 ]; then
    echo -e "${YELLOW}Skipped: $skipped${NC}"
fi
echo "================================================"

if [ $failed -gt 0 ]; then
    exit 1
fi

exit 0
