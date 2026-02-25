#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${ROOT}/ci/tests/allocator/build"

mkdir -p "${BUILD_DIR}"

c++ -std=c++17 -Wall -Wextra -pedantic \
    -I"${ROOT}/ci/tests/allocator/include" \
    -I"${ROOT}" \
    "${ROOT}/ci/tests/allocator/src/memory_allocator_smoke.cpp" \
    -o "${BUILD_DIR}/memory_allocator_smoke"

"${BUILD_DIR}/memory_allocator_smoke"
echo "PASS: allocator smoke test"
