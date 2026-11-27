cmake_minimum_required(VERSION 4.4.3)

set(vendor_root "${CMAKE_CURRENT_LIST_DIR}/../LimenEngine/vendor")

function(prepare_vendor_file relative_path url expected_sha256)
    set(destination "${vendor_root}/${relative_path}")
    if (EXISTS "${destination}")
        if (IS_DIRECTORY "${destination}")
            message(FATAL_ERROR "Expected a dependency file at ${destination}")
        endif ()
        file(SIZE "${destination}" local_size)
        if (local_size EQUAL 0)
            message(FATAL_ERROR "Empty dependency file at ${destination}. Move it aside and rerun.")
        endif ()
        # 保留开发者已准备的本地副本，固定版本校验针对本脚本下载的文件。
        message(STATUS "Using existing ${relative_path}")
        return()
    endif ()

    get_filename_component(destination_dir "${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_dir}")
    string(RANDOM LENGTH 12 nonce)
    set(temporary "${destination}.${nonce}.download")

    message(STATUS "Downloading ${relative_path}")
    file(DOWNLOAD "${url}" "${temporary}"
        STATUS transfer_status
        TLS_VERIFY ON
        TIMEOUT 60
        INACTIVITY_TIMEOUT 20)
    list(GET transfer_status 0 transfer_code)
    if (NOT transfer_code EQUAL 0)
        file(REMOVE "${temporary}")
        list(GET transfer_status 1 transfer_error)
        message(FATAL_ERROR "Download failed for ${relative_path}: ${transfer_error}")
    endif ()

    file(SHA256 "${temporary}" actual_sha256)
    if (NOT actual_sha256 STREQUAL expected_sha256)
        file(REMOVE "${temporary}")
        message(FATAL_ERROR "SHA-256 mismatch for ${relative_path}: expected ${expected_sha256}, got ${actual_sha256}")
    endif ()

    # 两个入口同时执行时，后完成的下载不能覆盖已经就位的文件。
    file(RENAME "${temporary}" "${destination}" RESULT rename_result NO_REPLACE)
    if (NOT rename_result STREQUAL "0")
        file(REMOVE "${temporary}")
        if (NOT rename_result STREQUAL "NO_REPLACE")
            message(FATAL_ERROR "Could not install ${relative_path}: ${rename_result}")
        endif ()
    endif ()
endfunction()

# toml11 v4.4.0 单头文件发行版，以及 stb_truetype.h 的固定上游修订。
set(toml11_url "https://raw.githubusercontent.com/ToruNiina/toml11/be08ba2be2a964edcdb3d3e3ea8d100abc26f286")
prepare_vendor_file("toml11/LICENSE"
    "${toml11_url}/LICENSE"
    "b547d41695cdfbc1d054b5044578e476a0e9d756849f1e0cb3864cfc83e50a9e")
prepare_vendor_file("toml11/toml.hpp"
    "${toml11_url}/single_include/toml.hpp"
    "71a65e312d8375f4fdbca3b921c6e3d9255058ea964071dd36f674183047d260")
prepare_vendor_file("stb_truetype/stb_truetype.h"
    "https://raw.githubusercontent.com/nothings/stb/9f755b604db3492df2cbeb39bf84b81e3e3932e3/stb_truetype.h"
    "428cfe93520f2aed5e413a220b0db5bba5b17e2ebe5a64e4a8efa9eeeb67ec5f")
