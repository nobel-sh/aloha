#!/bin/bash

# Directory conventions:
#   tests/integration/pass/*.alo  - Tests that should compile successfully
#   tests/integration/error/*.alo - Tests that should fail compilation
#                                   Optional marker: // expect-error: diagnostic substring

set -e

BUILD_COMPILER=false
for arg in "$@"; do
    case $arg in
        --build|-b)
            BUILD_COMPILER=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -b, --build   Build the compiler before running tests"
            echo "  -h, --help    Show this help message"
            exit 0
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
COMPILER="$PROJECT_DIR/build/aloha"
INTEGRATION_DIR="$PROJECT_DIR/tests/integration"
PASS_DIR="$INTEGRATION_DIR/pass"
ERROR_DIR="$INTEGRATION_DIR/error"
TEMP_DIR=$(mktemp -d)
# Set ALOHA_DEV so compiler can find stdlib when running test from /tmp
export ALOHA_DEV="$PROJECT_DIR"
FIXTURES_DIR="$INTEGRATION_DIR/fixtures"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "================================================"
echo "  Aloha Test Runner"
echo "================================================"
echo "Temporary directory: $TEMP_DIR"
echo ""

if [ "$BUILD_COMPILER" = true ]; then
    echo "Building compiler..."
    if ! cmake --build "$PROJECT_DIR/build" --target aloha > /dev/null 2>&1; then
        echo -e "${RED}Error: Build failed${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Build successful${NC}"
    echo ""
fi

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

if [ -f "$SCRIPT_DIR/stdlib.o" ]; then
    cp "$SCRIPT_DIR/stdlib.o" "$TEMP_DIR/"
fi

if [ -d "$FIXTURES_DIR" ]; then
    cp -R "$FIXTURES_DIR" "$TEMP_DIR/"
fi

total=0
passed=0
failed=0

run_error_test() {
    local file="$1"
    local name
    name="$(basename "$file")"

    total=$((total + 1))

    echo -n "Testing error/$name (expect error)... "
    expected_error=$(grep -m 1 'expect-error:' "$file" | sed 's/.*expect-error:[[:space:]]*//')

    if compile_output=$("$COMPILER" "$file" --no-link 2>&1); then
        echo -e "${RED}✗ UNEXPECTED SUCCESS${NC}"
        echo "  Expected compilation to fail but it succeeded"
        failed=$((failed + 1))
    else
        if [[ -n "$expected_error" ]] && ! echo "$compile_output" | grep -Fq "$expected_error"; then
            echo -e "${RED}✗ WRONG ERROR${NC}"
            echo "  Expected diagnostic containing: $expected_error"
            echo "$compile_output" | grep -E "Error|error|Expected" | head -3
            failed=$((failed + 1))
            return
        fi

        echo -e "${BLUE}✓ FAIL AS EXPECTED${NC}"
        passed=$((passed + 1))
    fi
}

run_pass_test() {
    local file="$1"
    local name
    name="$(basename "$file")"

    total=$((total + 1))

    echo -n "Testing pass/$name... "

    local work_file="$TEMP_DIR/$name"
    cp "$file" "$work_file"

    if ! compile_output=$("$COMPILER" "$work_file" 2>&1); then
        echo -e "${RED}✗ COMPILATION FAILED${NC}"
        echo "$compile_output" | grep -E "Error|error|Expected" | head -3
        failed=$((failed + 1))
        return
    fi

    if ! echo "$compile_output" | grep -q "Linking successful"; then
        echo -e "${RED}✗ LINKING FAILED${NC}"
        echo "$compile_output" | grep -E "error" | head -2
        failed=$((failed + 1))
        return
    fi

    echo -e "${GREEN}✓ PASS${NC}"
    passed=$((passed + 1))
}

while IFS= read -r file; do
    run_error_test "$file"
done < <(find "$ERROR_DIR" -maxdepth 1 -name "*.alo" -type f | sort)

while IFS= read -r file; do
    run_pass_test "$file"
done < <(find "$PASS_DIR" -maxdepth 1 -name "*.alo" -type f | sort)

echo ""
echo "================================================"
echo "  Test Summary"
echo "================================================"
echo -e "Total:   $total"
echo -e "${GREEN}Passed:  $passed${NC}"
if [ $failed -gt 0 ]; then
    echo -e "${RED}Failed:  $failed${NC}"
fi
echo "================================================"

if [ $failed -gt 0 ]; then
    exit 1
fi

exit 0
