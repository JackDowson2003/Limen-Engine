#!/usr/bin/env bash

set -euo pipefail

LIMEN_SCRIPT_DIRECTORY="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
LIMEN_PROJECT_ROOT="$(CDPATH= cd -- "${LIMEN_SCRIPT_DIRECTORY}/.." && pwd)"
LIMEN_SHARED_BUILD_DIRECTORY="${LIMEN_PROJECT_ROOT}/out/cmake-build-shared-test"
LIMEN_EXECUTABLE="${LIMEN_PROJECT_ROOT}/out/LimenSandBox"
LIMEN_SHARED_LIBRARY="${LIMEN_SHARED_BUILD_DIRECTORY}/LimenEngine/libLimenEngine.dylib"
LIMEN_RUN_SECONDS="${LIMEN_SHARED_RUN_SECONDS:-2}"
LIMEN_TEST_PROCESS_ID=""

cleanup() {
    if [[ -n "${LIMEN_TEST_PROCESS_ID}" ]] && kill -0 "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null; then
        kill -TERM "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
        wait "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

for LIMEN_REQUIRED_COMMAND in cmake ninja otool; do
    if ! command -v "${LIMEN_REQUIRED_COMMAND}" >/dev/null 2>&1; then
        echo "错误：找不到命令 ${LIMEN_REQUIRED_COMMAND}" >&2
        exit 1
    fi
done

echo "[1/4] 配置 LimenEngine SHARED Debug 构建"
cmake \
    -S "${LIMEN_PROJECT_ROOT}" \
    -B "${LIMEN_SHARED_BUILD_DIRECTORY}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLIMEN_ENGINE_SHARED=ON \
    -DLIMEN_WARNINGS_AS_ERRORS=OFF \
    -DLIMEN_ENABLE_SANITIZERS=ON

echo "[2/4] 编译 LimenSandBox"
cmake --build "${LIMEN_SHARED_BUILD_DIRECTORY}" --target LimenSandBox --parallel

echo "[3/4] 检查动态库和链接关系"
if [[ ! -f "${LIMEN_SHARED_LIBRARY}" ]]; then
    echo "错误：没有生成 ${LIMEN_SHARED_LIBRARY}" >&2
    exit 1
fi

if [[ ! -x "${LIMEN_EXECUTABLE}" ]]; then
    echo "错误：没有生成可执行文件 ${LIMEN_EXECUTABLE}" >&2
    exit 1
fi

if ! otool -L "${LIMEN_EXECUTABLE}" | grep -Fq "libLimenEngine.dylib"; then
    echo "错误：LimenSandBox 没有链接 libLimenEngine.dylib" >&2
    exit 1
fi

otool -L "${LIMEN_EXECUTABLE}"

echo "[4/4] 启动 LimenSandBox，${LIMEN_RUN_SECONDS} 秒后自动停止"
"${LIMEN_EXECUTABLE}" &
LIMEN_TEST_PROCESS_ID=$!
sleep "${LIMEN_RUN_SECONDS}"

if kill -0 "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null; then
    kill -TERM "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
    wait "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
else
    if ! wait "${LIMEN_TEST_PROCESS_ID}"; then
        echo "错误：LimenSandBox 运行过程中异常退出" >&2
        LIMEN_TEST_PROCESS_ID=""
        exit 1
    fi
fi

LIMEN_TEST_PROCESS_ID=""
trap - EXIT INT TERM

echo "SHARED 测试通过：已成功生成、链接并运行 libLimenEngine.dylib"
