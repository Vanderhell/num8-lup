cmake_minimum_required(VERSION 3.16)
project(num8lora_package_consumer C)

find_package(num8lora CONFIG REQUIRED)
set(NUM8LORA_C_STANDARD 99 CACHE STRING "C language standard for consumer targets")

function(num8lora_consumer_c99 target_name)
    set_target_properties(${target_name} PROPERTIES
        C_STANDARD ${NUM8LORA_C_STANDARD}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)
endfunction()

add_executable(package_consumer_legacy legacy_consumer.c)
num8lora_consumer_c99(package_consumer_legacy)
target_link_libraries(package_consumer_legacy PRIVATE num8lora::legacy)

add_executable(package_consumer_async async_consumer.c)
num8lora_consumer_c99(package_consumer_async)
target_link_libraries(package_consumer_async PRIVATE num8lora::sender num8lora::receiver)

if(NUM8LORA_EXPECT_HOSTED_METADATA)
    if(NOT TARGET num8lora::metadata)
        message(FATAL_ERROR "num8lora::metadata is missing from the installed package")
    endif()
    add_executable(package_consumer_metadata metadata_consumer.c)
    num8lora_consumer_c99(package_consumer_metadata)
    target_link_libraries(package_consumer_metadata PRIVATE num8lora::metadata)
else()
    if(TARGET num8lora::metadata)
        message(FATAL_ERROR "num8lora::metadata should not be exported when hosted metadata is disabled")
    endif()
endif()
