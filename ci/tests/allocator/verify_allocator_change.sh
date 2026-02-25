#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SMOKE_SCRIPT="${ROOT}/ci/tests/allocator/run_allocator_smoke.sh"

PROVES_CORE_REFERENCE_DIR="${PROVES_CORE_REFERENCE_DIR:-${ROOT}/../proves-core-reference}"
BUILD_CACHE="${BUILD_CACHE:-/tmp/proves-kmalloc-build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/tmp/proves-kmalloc-install}"

"${SMOKE_SCRIPT}"

if [[ ! -d "${PROVES_CORE_REFERENCE_DIR}" ]]; then
    echo "SKIP: integration build (missing repo: ${PROVES_CORE_REFERENCE_DIR})"
    exit 0
fi

VENV_ACTIVATE="${PROVES_CORE_REFERENCE_DIR}/fprime-venv/bin/activate"
if [[ ! -f "${VENV_ACTIVATE}" ]]; then
    echo "SKIP: integration build (missing venv: ${VENV_ACTIVATE})"
    exit 0
fi

if [[ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ]]; then
    echo "SKIP: integration build (ZEPHYR_SDK_INSTALL_DIR is not set)"
    echo "Set it to your Zephyr SDK install path, then rerun this script."
    exit 0
fi

if [[ ! -f "${ZEPHYR_SDK_INSTALL_DIR}/cmake/Zephyr-sdkConfig.cmake" && ! -f "${ZEPHYR_SDK_INSTALL_DIR}/Zephyr-sdkConfig.cmake" ]]; then
    echo "SKIP: integration build (invalid ZEPHYR_SDK_INSTALL_DIR: ${ZEPHYR_SDK_INSTALL_DIR})"
    exit 0
fi

source "${VENV_ACTIVATE}"

PYTHON_BIN="$(python -c 'import sys; print(sys.executable)')"
LIB_PATHS="${ROOT};${PROVES_CORE_REFERENCE_DIR}/lib/fprime-extras"

fprime-util generate -r "${PROVES_CORE_REFERENCE_DIR}" \
    -p "${PROVES_CORE_REFERENCE_DIR}" \
    --build-cache "${BUILD_CACHE}" \
    -f \
    -DFPRIME_LIBRARY_LOCATIONS="${LIB_PATHS}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DPython3_EXECUTABLE="${PYTHON_BIN}"

fprime-util build -r "${PROVES_CORE_REFERENCE_DIR}" \
    -p "${PROVES_CORE_REFERENCE_DIR}" \
    --build-cache "${BUILD_CACHE}"

echo "PASS: integration generate/build completed"
