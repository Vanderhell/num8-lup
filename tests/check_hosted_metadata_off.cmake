if(NOT DEFINED NUM8LORA_SOURCE_DIR OR NOT DEFINED NUM8LORA_TEST_BUILD_DIR)
    message(FATAL_ERROR "missing test paths")
endif()

file(REMOVE_RECURSE "${NUM8LORA_TEST_BUILD_DIR}")
set(configure_cmd_args
    -S "${NUM8LORA_SOURCE_DIR}"
    -B "${NUM8LORA_TEST_BUILD_DIR}"
    -DNUM8LORA_BUILD_HOSTED_METADATA=OFF
    -DNUM8LORA_BUILD_NUM8_ADAPTER=OFF)
if(WIN32)
    list(APPEND configure_cmd_args -DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=10.0.22621.0)
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" ${configure_cmd_args}
    RESULT_VARIABLE configure_result)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "hosted-metadata-off configure failed")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${NUM8LORA_TEST_BUILD_DIR}" --config Release --target num8lora_test num8lora_runtime_test num8lora_metadata_test num8lora_adapter_config_test
    RESULT_VARIABLE build_result)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "hosted-metadata-off core build failed")
endif()
