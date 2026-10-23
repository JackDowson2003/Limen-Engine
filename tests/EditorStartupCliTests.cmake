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
