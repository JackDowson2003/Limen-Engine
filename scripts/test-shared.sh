#!/usr/bin/env bash

set -euo pipefail

LIMEN_PROJECT_ROOT="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
LIMEN_OUTPUT_DIRECTORY="${LIMEN_PROJECT_ROOT}/out"
LIMEN_SHARED_BUILD_DIRECTORY="${LIMEN_OUTPUT_DIRECTORY}/cmake-build-shared-debug-clang17"
LIMEN_EXECUTABLE="${LIMEN_OUTPUT_DIRECTORY}/LimenSandBox"
LIMEN_SHARED_LIBRARY="${LIMEN_SHARED_BUILD_DIRECTORY}/LimenEngine/libLimenEngine.dylib"
LIMEN_RUN_SECONDS="${LIMEN_SHARED_RUN_SECONDS:-2}"
LIMEN_TEST_PROCESS_ID=""

cleanup() {
    if [[ -n "${LIMEN_TEST_PROCESS_ID}" ]] && kill -0 "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null; then
        kill -TERM "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
        wait "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
    fi
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if [[ "$(uname -s)" != Darwin ]]; then
    echo "错误：Shared Debug 冒烟测试需要 macOS。" >&2
    exit 1
fi

if [[ ! "${LIMEN_RUN_SECONDS}" =~ ^([0-9]+([.][0-9]+)?|[.][0-9]+)$ ||
      ! "${LIMEN_RUN_SECONDS}" =~ [1-9] ]]; then
    echo "错误：LIMEN_SHARED_RUN_SECONDS 必须是大于 0 的秒数。" >&2
    exit 2
fi

for LIMEN_REQUIRED_COMMAND in cmake ninja otool; do
    if ! command -v "${LIMEN_REQUIRED_COMMAND}" >/dev/null 2>&1; then
        echo "错误：找不到命令 ${LIMEN_REQUIRED_COMMAND}" >&2
        exit 1
    fi
done

cd -- "${LIMEN_PROJECT_ROOT}"
echo "[1/4] 配置 LimenEngine Shared Debug 构建"
cmake --preset ninja-shared-debug

echo "[2/4] 编译 LimenSandBox"
cmake --build --preset build-shared-debug

echo "[3/4] 检查动态库和链接关系"
if [[ ! -f "${LIMEN_SHARED_LIBRARY}" ]]; then
    echo "错误：没有生成 ${LIMEN_SHARED_LIBRARY}" >&2
    exit 1
fi

if [[ ! -x "${LIMEN_EXECUTABLE}" ]]; then
    echo "错误：没有生成可执行文件 ${LIMEN_EXECUTABLE}" >&2
    exit 1
fi

LIMEN_LINKED_LIBRARIES="$(otool -L "${LIMEN_EXECUTABLE}")"
if [[ "${LIMEN_LINKED_LIBRARIES}" != *"libLimenEngine.dylib"* ]]; then
    # 各 Preset 共用 Sandbox 输出，静态构建可能覆盖了 Ninja 记录的可执行文件。
    # 只更新时间戳来触发 Shared 重新链接，保留已有的构建文件和源码。
    cmake -E touch "${LIMEN_SHARED_LIBRARY}"
    cmake --build --preset build-shared-debug
    LIMEN_LINKED_LIBRARIES="$(otool -L "${LIMEN_EXECUTABLE}")"
fi
if [[ "${LIMEN_LINKED_LIBRARIES}" != *"libLimenEngine.dylib"* ]]; then
    echo "错误：LimenSandBox 没有链接 libLimenEngine.dylib" >&2
    exit 1
fi

printf '%s\n' "${LIMEN_LINKED_LIBRARIES}"

echo "[4/4] 启动 LimenSandBox，${LIMEN_RUN_SECONDS} 秒后自动停止"
cd -- "${LIMEN_OUTPUT_DIRECTORY}"
./LimenSandBox &
LIMEN_TEST_PROCESS_ID=$!
sleep "${LIMEN_RUN_SECONDS}"

if ! kill -0 "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null; then
    LIMEN_EXIT_STATUS=0
    wait "${LIMEN_TEST_PROCESS_ID}" || LIMEN_EXIT_STATUS=$?
    LIMEN_TEST_PROCESS_ID=""
    echo "错误：LimenSandBox 未运行满 ${LIMEN_RUN_SECONDS} 秒（退出码 ${LIMEN_EXIT_STATUS}）。" >&2
    exit 1
fi

# 只停止本脚本启动的进程，区分预期的 SIGTERM 和运行时崩溃。
kill -TERM "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || true
LIMEN_EXIT_STATUS=0
wait "${LIMEN_TEST_PROCESS_ID}" 2>/dev/null || LIMEN_EXIT_STATUS=$?
LIMEN_TEST_PROCESS_ID=""
if [[ "${LIMEN_EXIT_STATUS}" != 0 && "${LIMEN_EXIT_STATUS}" != 143 ]]; then
    echo "错误：LimenSandBox 异常退出（退出码 ${LIMEN_EXIT_STATUS}）。" >&2
    exit 1
fi

echo "SHARED 测试通过：已成功生成、链接并运行 libLimenEngine.dylib"
