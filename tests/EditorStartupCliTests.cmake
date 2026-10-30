# 只走参数预检的失败路径，不创建窗口或图形 Context。
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
set(test_root "${CMAKE_CURRENT_BINARY_DIR}/editor-startup-registry-${test_suffix}")
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
file(REMOVE_RECURSE "${test_root}")
if(NOT "${registry_result}" STREQUAL "1" OR
   NOT registry_log MATCHES "加载资产注册表失败" OR
   registry_log MATCHES "Create Window Title|Creating Window")
    message(FATAL_ERROR
        "invalid asset metadata did not fail before window creation (exit=${registry_result}):\n${registry_log}")
endif()
