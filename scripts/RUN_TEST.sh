#!/usr/bin/env bash
set -euo pipefail

LIMEN_PROJECT_ROOT="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
LIMEN_TEST_BUILD_DIRECTORY="${LIMEN_PROJECT_ROOT}/out/cmake-build-test-clang17"
LIMEN_ENABLE_GPU_TESTS=ON

if (( $# > 1 )) || [[ $# -eq 1 && "$1" != "--cpu-only" ]]; then
    echo "用法：bash scripts/RUN_TEST.sh [--cpu-only]" >&2
    exit 2
fi
if [[ $# -eq 1 ]]; then
    LIMEN_ENABLE_GPU_TESTS=OFF
fi

LIMEN_TEST_TARGETS=(
    LimenPostProcessReferenceTests
    LimenAxisAlignedBoundingBoxTests
    LimenAssetPathTests
)
LIMEN_TEST_NAMES='LimenPostProcessReference|LimenAxisAlignedBoundingBox|LimenAssetPath'
if [[ "${LIMEN_ENABLE_GPU_TESTS}" == ON ]]; then
    LIMEN_TEST_TARGETS+=(LimenOpenGLPostProcessTests LimenOpenGLRenderer2DPipelineTests)
    LIMEN_TEST_NAMES+='|LimenOpenGLPostProcess|LimenOpenGLRenderer2DPipeline'
fi

# 使用独立缓存，避免把测试开关写入 CLion 的 Debug 构建目录。
cd -- "${LIMEN_PROJECT_ROOT}"
cmake --preset ninja-debug -B "${LIMEN_TEST_BUILD_DIRECTORY}" \
    -DBUILD_TESTING=ON -DLIMEN_ENABLE_OPENGL_GPU_TESTS="${LIMEN_ENABLE_GPU_TESTS}"
cmake --build "${LIMEN_TEST_BUILD_DIRECTORY}" --target "${LIMEN_TEST_TARGETS[@]}"
ctest --test-dir "${LIMEN_TEST_BUILD_DIRECTORY}" \
    -R "^(${LIMEN_TEST_NAMES})$" --no-tests=error \
    --output-on-failure
