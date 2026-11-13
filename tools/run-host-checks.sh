#!/usr/bin/env bash
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output_root="${HOST_TEST_OUTPUT_ROOT:-$repo/build}"
mkdir -p "$output_root"
result="$(mktemp -d "$output_root/host.XXXXXX")"
cc="${HOST_CC:-clang}"
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
"$cc" --version > "$result/compiler.txt"
"$cc" -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer -g -DTEST_MODE=3 \
  "$repo/host-tests/test-observer.c" -o "$result/test-observer"
cd "$result"
./test-observer | tee observer.txt
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="$repo/tools" \
  python3 -B -m unittest discover -s "$repo/host-tests" -p 'test_*.py' -v
printf 'HOST_CHECKS=PASS\nResults: %s\n' "$result"
