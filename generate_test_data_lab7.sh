#!/usr/bin/env bash
set -e


BAYAN_BIN="./bayan_lab7.exe"
if [ ! -f "$BAYAN_BIN" ]; then
    BAYAN_BIN="$(pwd)/bayan_lab7.exe"
fi

if [ ! -f "$BAYAN_BIN" ]; then
    echo "Error: cannot find executable file."
    exit 1
fi

TEST_ROOT="./test_playground"

echo "1. Cleaning directory structure (up to 4 levels)."
rm -rf "$TEST_ROOT"

mkdir -p "$TEST_ROOT/dir_a/sub_1/sub_2/sub_3"
mkdir -p "$TEST_ROOT/dir_b/sub_1"
mkdir -p "$TEST_ROOT/excluded_dir/deep"

echo "2. Generating raw files using dd if=/dev/urandom"

dd if=/dev/urandom of="$TEST_ROOT/orig_1.bin" bs=2500 count=1 status=none
cp "$TEST_ROOT/orig_1.bin" "$TEST_ROOT/dir_a/sub_1/copy_1_a.bin"
cp "$TEST_ROOT/orig_1.bin" "$TEST_ROOT/dir_b/sub_1/copy_1_b.BIN"

# these files must be excluded from resulting output
dd if=/dev/urandom of="$TEST_ROOT/dir_a/sub_1/sub_2/orig_2.dat" bs=4096 count=1 status=none
cp "$TEST_ROOT/dir_a/sub_1/sub_2/orig_2.dat" "$TEST_ROOT/dir_a/sub_1/sub_2/sub_3/copy_2_deep.dat"

cp "$TEST_ROOT/orig_1.bin" "$TEST_ROOT/excluded_dir/deep/ignored_copy.bin"

dd if=/dev/urandom of="$TEST_ROOT/unique_1.bin" bs=2500 count=1 status=none
dd if=/dev/urandom of="$TEST_ROOT/dir_a/unique_2.dat" bs=1500 count=1 status=none

dd if=/dev/urandom of="$TEST_ROOT/tiny_1.bin" bs=1 count=1 status=none
cp "$TEST_ROOT/tiny_1.bin" "$TEST_ROOT/tiny_2.bin"

echo "Test directory structure is ready:"
find "$TEST_ROOT" -type f | sort
echo ""

echo "Test 1: Full scan (depth 4, blok size 512 bytes, with excluded dir)"
"$BAYAN_BIN" \
    -i "$TEST_ROOT" \
    -e "$TEST_ROOT/excluded_dir" \
    -d 4 \
    -s 512

echo ""
echo "Test 2: Scan with max depth (depth = 2 — duplicated file is copy_2_deep.dat/ and it should not be found)"
"$BAYAN_BIN" \
    -i "$TEST_ROOT" \
    -e "$TEST_ROOT/excluded_dir" \
    -d 2 \
    -s 512

echo ""
echo "Test 3: Filter with mask (*.bin)"
"$BAYAN_BIN" \
    -i "$TEST_ROOT" \
    -e "$TEST_ROOT/excluded_dir" \
    -d 4 \
    -f "*.bin" \
    -s 1024
