if(NOT DEFINED NUM8LORA_SOURCE_DIR OR NOT DEFINED NUM8LORA_TEST_BUILD_DIR OR NOT DEFINED NUM8LORA_BUILD_DIR OR NOT DEFINED NUM8LORA_EXPECT_HOSTED_METADATA)
    message(FATAL_ERROR "missing test paths")
endif()

set(install_prefix "${NUM8LORA_TEST_BUILD_DIR}/install")
set(consumer_build "${NUM8LORA_TEST_BUILD_DIR}/consumer-build")

file(REMOVE_RECURSE "${NUM8LORA_TEST_BUILD_DIR}")
file(MAKE_DIRECTORY "${NUM8LORA_TEST_BUILD_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${NUM8LORA_BUILD_DIR}" --config Release --prefix "${install_prefix}"
    RESULT_VARIABLE install_result)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "package install failed")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${NUM8LORA_SOURCE_DIR}/tests/package_consumer"
        -B "${consumer_build}"
        -DCMAKE_PREFIX_PATH=${install_prefix}
        -DNUM8LORA_EXPECT_HOSTED_METADATA=${NUM8LORA_EXPECT_HOSTED_METADATA}
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

execute_process(
    COMMAND "${consumer_build}/Release/package_consumer_legacy.exe"
    RESULT_VARIABLE legacy_run_result)
if(NOT legacy_run_result EQUAL 0)
    message(FATAL_ERROR "legacy consumer run failed")
endif()

execute_process(
    COMMAND "${consumer_build}/Release/package_consumer_async.exe"
    RESULT_VARIABLE async_run_result)
if(NOT async_run_result EQUAL 0)
    message(FATAL_ERROR "async consumer run failed")
endif()

if(NUM8LORA_EXPECT_HOSTED_METADATA)
    execute_process(
        COMMAND "${consumer_build}/Release/package_consumer_metadata.exe"
        RESULT_VARIABLE metadata_run_result)
    if(NOT metadata_run_result EQUAL 0)
        message(FATAL_ERROR "metadata consumer run failed")
    endif()
endif()
