# 覆盖创建窗口或图形 Context 之前的启动失败路径。
if(NOT DEFINED LIMEN_EDITOR_EXECUTABLE)
    message(FATAL_ERROR "LIMEN_EDITOR_EXECUTABLE is required")
endif()

execute_process(
    COMMAND "${LIMEN_EDITOR_EXECUTABLE}" --project missing.invalid
    RESULT_VARIABLE open_result
    OUTPUT_VARIABLE open_output
    ERROR_VARIABLE open_error
    TIMEOUT 5
)

set(open_log "${open_output}${open_error}")
if(NOT "${open_result}" STREQUAL "1" OR
   NOT open_log MATCHES "打开项目失败")
    message(FATAL_ERROR
        "--project did not reach project loading (exit=${open_result}):\n${open_log}")
endif()

execute_process(
    COMMAND "${LIMEN_EDITOR_EXECUTABLE}" --project missing.invalid --scene test.lscene
    RESULT_VARIABLE scene_result
    OUTPUT_VARIABLE scene_output
    ERROR_VARIABLE scene_error
    TIMEOUT 5
)

set(scene_log "${scene_output}${scene_error}")
if(NOT "${scene_result}" STREQUAL "1" OR
   NOT scene_log MATCHES "打开项目失败")
    message(FATAL_ERROR
        "--project with --scene did not reach project loading (exit=${scene_result}):\n${scene_log}")
endif()

execute_process(
    COMMAND "${LIMEN_EDITOR_EXECUTABLE}" --create-project . "CLI Test"
    RESULT_VARIABLE create_result
    OUTPUT_VARIABLE create_output
    ERROR_VARIABLE create_error
    TIMEOUT 5
)

set(create_log "${create_output}${create_error}")
if(NOT "${create_result}" STREQUAL "1" OR
   NOT create_log MATCHES "创建项目配置失败" OR
   NOT create_log MATCHES "项目目录必须是绝对路径")
    message(FATAL_ERROR
        "--create-project did not reach path validation (exit=${create_result}):\n${create_log}")
endif()

# 合法项目配置 + 坏 sidecar 应在 CreateApplication 阶段失败，不能走到窗口创建。
# 随机目录仅供此用例使用；清理时不触碰其他测试或用户项目。
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef test_suffix)
set(test_root "${CMAKE_CURRENT_BINARY_DIR}/editor-startup-cli-${test_suffix}")
if(EXISTS "${test_root}")
    message(FATAL_ERROR "Test directory already exists: ${test_root}")
endif()
file(MAKE_DIRECTORY "${test_root}/assets")
set(project_file "${test_root}/Game1.lmproject")
file(WRITE "${project_file}"
    "schema_version = 1\n"
    "project_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\n"
    "display_name = \"Editor Startup Test\"\n"
    "asset_root = \"assets\"\n")
file(WRITE "${test_root}/assets/bad.bin" "asset bytes")
file(WRITE "${test_root}/assets/bad.bin.lmmeta"
    "schema_version = 1\nasset_id = [\n")

execute_process(
    COMMAND "${LIMEN_EDITOR_EXECUTABLE}" --project "${project_file}"
    RESULT_VARIABLE registry_result
    OUTPUT_VARIABLE registry_output
    ERROR_VARIABLE registry_error
    TIMEOUT 5
)

set(registry_log "${registry_output}${registry_error}")
if(NOT "${registry_result}" STREQUAL "1" OR
   NOT registry_log MATCHES "加载资产注册表失败" OR
   registry_log MATCHES "Create Window Title|Creating Window")
    file(REMOVE_RECURSE "${test_root}")
    message(FATAL_ERROR
        "invalid asset metadata did not fail before window creation (exit=${registry_result}):\n${registry_log}")
endif()

# 从其他工作目录启动，配置与资产仍应从项目文件所在目录解析。
file(REMOVE "${test_root}/assets/bad.bin" "${test_root}/assets/bad.bin.lmmeta")
file(MAKE_DIRECTORY "${test_root}/Config" "${test_root}/working")
set(default_game_file "${test_root}/Config/DefaultGame.toml")

function(expect_invalid_startup_scene label contents key reason)
    file(WRITE "${default_game_file}" "${contents}")
    execute_process(
        COMMAND "${LIMEN_EDITOR_EXECUTABLE}" --project "${project_file}"
        WORKING_DIRECTORY "${test_root}/working"
        RESULT_VARIABLE startup_result
        OUTPUT_VARIABLE startup_output
        ERROR_VARIABLE startup_error
        TIMEOUT 5
    )
    set(startup_log "${startup_output}${startup_error}")
    string(FIND "${startup_log}" "key='${key}'" key_position)
    string(FIND "${startup_log}" "${reason}" reason_position)
    if(NOT "${startup_result}" STREQUAL "1" OR
       NOT startup_log MATCHES "DefaultGame[.]toml" OR
       key_position EQUAL -1 OR reason_position EQUAL -1 OR
       startup_log MATCHES "Create Window Title|Creating Window|Creating window")
        file(REMOVE_RECURSE "${test_root}")
        message(FATAL_ERROR "${label} did not fail at startup scene validation (exit=${startup_result}):\n${startup_log}")
    endif()
endfunction()

expect_invalid_startup_scene("malformed game config" "schema_version = [\n" "" "")
expect_invalid_startup_scene("unsupported game config schema" "schema_version = 2\n" "schema_version" "")

set(scene_key "game.startup_scene_asset_id")
set(scene_config_prefix "schema_version = 1\n[game]\nstartup_scene_asset_id = ")
set(scene_id "bd701ffd-3899-41e6-b82b-35ca5c850494")
set(scene_config "${scene_config_prefix}\"${scene_id}\"\n")
expect_invalid_startup_scene("malformed startup AssetID" "${scene_config_prefix}\"invalid\"\n"
    "${scene_key}" "启动场景必须是有效的非零 AssetID 字符串")
expect_invalid_startup_scene("unregistered startup AssetID" "${scene_config}"
    "${scene_key}" "启动场景 AssetID 未在项目资产注册表中登记")

set(scene_metadata "schema_version = 1\nasset_id = \"${scene_id}\"\n")
file(WRITE "${test_root}/assets/Missing.lscene.lmmeta" "${scene_metadata}")
expect_invalid_startup_scene("missing startup scene source" "${scene_config}"
    "${scene_key}" "启动场景资产文件不存在")

file(REMOVE "${test_root}/assets/Missing.lscene.lmmeta")
file(WRITE "${test_root}/assets/NotAScene.txt" "asset bytes")
file(WRITE "${test_root}/assets/NotAScene.txt.lmmeta" "${scene_metadata}")
expect_invalid_startup_scene("non-scene startup asset" "${scene_config}"
    "${scene_key}" "启动场景资产必须是 .lscene 文件")

file(REMOVE_RECURSE "${test_root}")
