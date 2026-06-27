if(NOT DEFINED NUM8LORA_SOURCE_DIR OR NOT DEFINED NUM8LORA_TEST_BUILD_DIR OR NOT DEFINED NUM8LORA_BUILD_DIR OR NOT DEFINED NUM8LORA_EXPECT_HOSTED_METADATA)
    message(FATAL_ERROR "missing test paths")
endif()

set(install_prefix "${NUM8LORA_TEST_BUILD_DIR}/install")
set(consumer_build "${NUM8LORA_TEST_BUILD_DIR}/consumer-build")
set(consumer_bin_dir "${consumer_build}")
set(consumer_exe_suffix "")
if(WIN32)
    set(consumer_exe_suffix ".exe")
endif()

file(REMOVE_RECURSE "${NUM8LORA_TEST_BUILD_DIR}")
file(MAKE_DIRECTORY "${NUM8LORA_TEST_BUILD_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${NUM8LORA_BUILD_DIR}" --config Release --prefix "${install_prefix}"
    RESULT_VARIABLE install_result)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "package install failed")
endif()

set(consumer_configure_args
    -S "${NUM8LORA_SOURCE_DIR}/tests/package_consumer"
    -B "${consumer_build}"
    -DCMAKE_PREFIX_PATH=${install_prefix}
    -DNUM8LORA_EXPECT_HOSTED_METADATA=${NUM8LORA_EXPECT_HOSTED_METADATA}
)

if(DEFINED NUM8LORA_CONSUMER_C_FLAGS AND NOT NUM8LORA_CONSUMER_C_FLAGS STREQUAL "")
    list(APPEND consumer_configure_args "-DCMAKE_C_FLAGS=${NUM8LORA_CONSUMER_C_FLAGS}")
endif()

if(DEFINED NUM8LORA_CONSUMER_EXE_LINKER_FLAGS AND NOT NUM8LORA_CONSUMER_EXE_LINKER_FLAGS STREQUAL "")
    list(APPEND consumer_configure_args "-DCMAKE_EXE_LINKER_FLAGS=${NUM8LORA_CONSUMER_EXE_LINKER_FLAGS}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" ${consumer_configure_args}
    RESULT_VARIABLE configure_result)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "consumer configure failed")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}" --config Release
    RESULT_VARIABLE build_result)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "consumer build failed")
endif()

if(EXISTS "${consumer_build}/Release")
    set(consumer_bin_dir "${consumer_build}/Release")
endif()

execute_process(
    COMMAND "${consumer_bin_dir}/package_consumer_legacy${consumer_exe_suffix}"
    RESULT_VARIABLE legacy_run_result)
if(NOT legacy_run_result EQUAL 0)
    message(FATAL_ERROR "legacy consumer run failed")
endif()

execute_process(
    COMMAND "${consumer_bin_dir}/package_consumer_async${consumer_exe_suffix}"
    RESULT_VARIABLE async_run_result)
if(NOT async_run_result EQUAL 0)
    message(FATAL_ERROR "async consumer run failed")
endif()

if(NUM8LORA_EXPECT_HOSTED_METADATA)
    execute_process(
        COMMAND "${consumer_bin_dir}/package_consumer_metadata${consumer_exe_suffix}"
        RESULT_VARIABLE metadata_run_result)
    if(NOT metadata_run_result EQUAL 0)
        message(FATAL_ERROR "metadata consumer run failed")
    endif()
endif()
