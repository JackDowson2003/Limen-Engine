git submodule update --init --recursive

# 构建环境：下载 stb_truetype 单头文件，已存在则跳过。
LIMEN_STB_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)/../LimenEngine/vendor/stb_truetype"
mkdir -p "${LIMEN_STB_DIR}"
if [ ! -f "${LIMEN_STB_DIR}/stb_truetype.h" ]; then
    curl -L --fail -o "${LIMEN_STB_DIR}/stb_truetype.h" \
        https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h \
        || exit 1
fi
