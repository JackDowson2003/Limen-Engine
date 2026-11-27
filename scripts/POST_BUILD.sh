#!/usr/bin/env bash
set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
assets_dir="${repo_root}/out/assets"

# 与 LimenSandBoxAssets 顺序相同，同名资源由 Sandbox 版本覆盖。
cmake -E copy_directory_if_different "${repo_root}/LimenEngine/assets" "${assets_dir}"
cmake -E copy_directory_if_different "${repo_root}/LimenSandBox/assets" "${assets_dir}"
