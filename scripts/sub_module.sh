#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(CDPATH= cd -- "${script_dir}/.." && pwd)"

for tool in git cmake; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "错误：找不到 ${tool}，请先准备项目工具链。" >&2
        exit 1
    fi
done

git -C "${repo_root}" submodule update --init --recursive
cmake -P "${script_dir}/prepare-vendor-headers.cmake"
