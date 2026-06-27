cmake_minimum_required(VERSION 3.16)
project(num8_lup C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

set(NUM8LORA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(NUM8LORA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NUM8LORA_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

option(NUM8LORA_BUILD_HOSTED_METADATA "Build hosted metadata persistence helpers" ON)
option(NUM8LORA_BUILD_NUM8_ADAPTER "Build the optional NUM8 adapter" OFF)
set(NUM8LORA_NUM8_ROOT "" CACHE PATH "Path to the NUM8 dependency used by the optional adapter")

add_library(num8lora_metadata_core STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_metadata.c
)
target_include_directories(num8lora_metadata_core PUBLIC ${NUM8LORA_INCLUDE_DIR})

# Legacy combined library
add_library(num8lora_static STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora.c
    ${NUM8LORA_SRC_DIR}/num8lora_runtime.c
)
target_include_directories(num8lora_static PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_link_libraries(num8lora_static PUBLIC num8lora_metadata_core)

add_library(num8lora_shared SHARED
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora.c
    ${NUM8LORA_SRC_DIR}/num8lora_runtime.c
    ${NUM8LORA_SRC_DIR}/num8lora_metadata.c
)
target_include_directories(num8lora_shared PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_compile_definitions(num8lora_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
set_target_properties(num8lora_shared PROPERTIES OUTPUT_NAME num8lora WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Async split protocol libraries
add_library(num8lora_sender_static STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_sender.c
)
target_include_directories(num8lora_sender_static PUBLIC ${NUM8LORA_INCLUDE_DIR})

add_library(num8lora_sender_shared SHARED
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_sender.c
)
target_include_directories(num8lora_sender_shared PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_compile_definitions(num8lora_sender_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
set_target_properties(num8lora_sender_shared PROPERTIES OUTPUT_NAME num8lora_sender WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(num8lora_receiver_static STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_receiver.c
)
target_include_directories(num8lora_receiver_static PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_link_libraries(num8lora_receiver_static PUBLIC num8lora_metadata_core)

add_library(num8lora_receiver_shared SHARED
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_receiver.c
    ${NUM8LORA_SRC_DIR}/num8lora_metadata.c
)
target_include_directories(num8lora_receiver_shared PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_compile_definitions(num8lora_receiver_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
set_target_properties(num8lora_receiver_shared PROPERTIES OUTPUT_NAME num8lora_receiver WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(NUM8LORA_BUILD_HOSTED_METADATA)
    add_library(num8lora_metadata_hosted STATIC
        ${NUM8LORA_SRC_DIR}/num8lora_meta.c
    )
    target_include_directories(num8lora_metadata_hosted PUBLIC ${NUM8LORA_INCLUDE_DIR})
    target_compile_definitions(num8lora_metadata_hosted PUBLIC NUM8LORA_BUILD_HOSTED_METADATA=1)
    target_link_libraries(num8lora_metadata_hosted PUBLIC num8lora_metadata_core)
endif()

# Tests
add_executable(num8lora_test ${NUM8LORA_TEST_DIR}/num8lora_test.c)
target_link_libraries(num8lora_test PRIVATE num8lora_static)
target_compile_definitions(num8lora_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

add_executable(num8lora_runtime_test ${NUM8LORA_TEST_DIR}/num8lora_runtime_test.c)
target_link_libraries(num8lora_runtime_test PRIVATE num8lora_static)
target_compile_definitions(num8lora_runtime_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

add_executable(num8lora_op_test ${NUM8LORA_TEST_DIR}/num8lora_op_test.c)
target_link_libraries(num8lora_op_test PRIVATE num8lora_sender_static)
target_compile_definitions(num8lora_op_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

add_executable(num8lora_async_test ${NUM8LORA_TEST_DIR}/num8lora_async_test.c)
target_link_libraries(num8lora_async_test PRIVATE num8lora_sender_static num8lora_receiver_static)
target_compile_definitions(num8lora_async_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

add_executable(num8lora_metadata_test ${NUM8LORA_TEST_DIR}/num8lora_metadata_test.c)
target_link_libraries(num8lora_metadata_test PRIVATE num8lora_metadata_core)
target_compile_definitions(num8lora_metadata_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)
if(NUM8LORA_BUILD_HOSTED_METADATA)
    target_link_libraries(num8lora_metadata_test PRIVATE num8lora_metadata_hosted)
endif()

add_executable(num8lora_adapter_config_test ${NUM8LORA_TEST_DIR}/num8lora_adapter_config_test.c)
target_link_libraries(num8lora_adapter_config_test PRIVATE num8lora_static)
target_compile_definitions(num8lora_adapter_config_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

add_executable(num8lora_async_demo examples/async_demo.c)
target_link_libraries(num8lora_async_demo PRIVATE num8lora_sender_static num8lora_receiver_static)

add_executable(num8lora_codec_contract_test ${NUM8LORA_TEST_DIR}/num8lora_codec_contract_test.c)
target_link_libraries(num8lora_codec_contract_test PRIVATE num8lora_static num8lora_sender_static)
target_compile_definitions(num8lora_codec_contract_test PRIVATE
    NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
    NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

add_executable(num8lora_header_smoke_num8lora ${NUM8LORA_TEST_DIR}/header_smoke_num8lora.c)
target_include_directories(num8lora_header_smoke_num8lora PRIVATE ${NUM8LORA_INCLUDE_DIR})

add_executable(num8lora_header_smoke_num8lora_op ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_op.c)
target_include_directories(num8lora_header_smoke_num8lora_op PRIVATE ${NUM8LORA_INCLUDE_DIR})

add_executable(num8lora_header_smoke_num8lora_metadata ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_metadata.c)
target_include_directories(num8lora_header_smoke_num8lora_metadata PRIVATE ${NUM8LORA_INCLUDE_DIR})

add_executable(num8lora_header_smoke_num8lora_sender ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_sender.c)
target_include_directories(num8lora_header_smoke_num8lora_sender PRIVATE ${NUM8LORA_INCLUDE_DIR})
target_link_libraries(num8lora_header_smoke_num8lora_sender PRIVATE num8lora_sender_static)

add_executable(num8lora_header_smoke_num8lora_receiver ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_receiver.c)
target_include_directories(num8lora_header_smoke_num8lora_receiver PRIVATE ${NUM8LORA_INCLUDE_DIR})
target_link_libraries(num8lora_header_smoke_num8lora_receiver PRIVATE num8lora_receiver_static)

add_executable(num8lora_header_smoke_repeat ${NUM8LORA_TEST_DIR}/header_smoke_repeat.c)
target_include_directories(num8lora_header_smoke_repeat PRIVATE ${NUM8LORA_INCLUDE_DIR})

add_executable(num8lora_header_smoke_cpp ${NUM8LORA_TEST_DIR}/header_smoke_cpp.cpp)
target_include_directories(num8lora_header_smoke_cpp PRIVATE ${NUM8LORA_INCLUDE_DIR})

add_executable(num8lora_header_smoke_mtu
    ${NUM8LORA_TEST_DIR}/header_smoke_mtu_a.c
    ${NUM8LORA_TEST_DIR}/header_smoke_mtu_b.c)
target_include_directories(num8lora_header_smoke_mtu PRIVATE ${NUM8LORA_INCLUDE_DIR})
target_link_libraries(num8lora_header_smoke_mtu PRIVATE num8lora_static)

enable_testing()
add_test(NAME num8lora_test COMMAND num8lora_test)
add_test(NAME num8lora_runtime_test COMMAND num8lora_runtime_test)
add_test(NAME num8lora_op_test COMMAND num8lora_op_test)
add_test(NAME num8lora_async_test COMMAND num8lora_async_test)
add_test(NAME num8lora_metadata_test COMMAND num8lora_metadata_test)
add_test(NAME num8lora_adapter_config_test COMMAND num8lora_adapter_config_test)
add_test(NAME num8lora_codec_contract_test COMMAND num8lora_codec_contract_test)
add_test(NAME num8lora_header_smoke_num8lora COMMAND num8lora_header_smoke_num8lora)
add_test(NAME num8lora_header_smoke_num8lora_op COMMAND num8lora_header_smoke_num8lora_op)
add_test(NAME num8lora_header_smoke_num8lora_metadata COMMAND num8lora_header_smoke_num8lora_metadata)
add_test(NAME num8lora_header_smoke_num8lora_sender COMMAND num8lora_header_smoke_num8lora_sender)
add_test(NAME num8lora_header_smoke_num8lora_receiver COMMAND num8lora_header_smoke_num8lora_receiver)
add_test(NAME num8lora_header_smoke_repeat COMMAND num8lora_header_smoke_repeat)
add_test(NAME num8lora_header_smoke_cpp COMMAND num8lora_header_smoke_cpp)
add_test(NAME num8lora_header_smoke_mtu COMMAND num8lora_header_smoke_mtu)

add_test(
    NAME num8lora_hosted_metadata_disabled_build
    COMMAND ${CMAKE_COMMAND}
        -DNUM8LORA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DNUM8LORA_TEST_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/hosted-metadata-off-check
        -P ${NUM8LORA_TEST_DIR}/check_hosted_metadata_off.cmake)

add_test(
    NAME num8lora_num8_adapter_requested_without_dependency
    COMMAND ${CMAKE_COMMAND}
        -DNUM8LORA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DNUM8LORA_TEST_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/num8-adapter-request-check
        -DNUM8LORA_MISSING_NUM8_ROOT=${CMAKE_CURRENT_BINARY_DIR}/missing-num8
        -P ${NUM8LORA_TEST_DIR}/check_num8_adapter_request_fail.cmake)

install(TARGETS
    num8lora_metadata_core
    num8lora_static
    num8lora_shared
    num8lora_sender_static
    num8lora_sender_shared
    num8lora_receiver_static
    num8lora_receiver_shared
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(NUM8LORA_BUILD_HOSTED_METADATA)
    install(TARGETS num8lora_metadata_hosted
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(FILES
    ${NUM8LORA_INCLUDE_DIR}/num8lora.h
    ${NUM8LORA_INCLUDE_DIR}/num8lora_metadata.h
    ${NUM8LORA_INCLUDE_DIR}/num8lora_op.h
    ${NUM8LORA_INCLUDE_DIR}/num8lora_sender.h
    ${NUM8LORA_INCLUDE_DIR}/num8lora_receiver.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES
    README.md
    LICENSE
    docs/QUICKSTART.md
    docs/ASYNC_SPLIT_API.md
    docs/LEGACY_DLL_API.md
    docs/WIRE_FORMAT.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/num8-lup
)

if(NUM8LORA_BUILD_NUM8_ADAPTER)
    if(NOT EXISTS "${NUM8LORA_NUM8_ROOT}/num8.c" OR NOT EXISTS "${NUM8LORA_NUM8_ROOT}/num8.h")
        message(FATAL_ERROR "NUM8LORA_BUILD_NUM8_ADAPTER is ON but NUM8LORA_NUM8_ROOT does not contain num8.c and num8.h")
    endif()

    add_library(num8lora_receiver_num8 STATIC
        ${NUM8LORA_SRC_DIR}/num8lora_num8_bridge.c
        ${NUM8LORA_NUM8_ROOT}/num8.c
    )
    add_library(num8lora::receiver_num8 ALIAS num8lora_receiver_num8)
    target_compile_definitions(num8lora_receiver_num8 PUBLIC NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_receiver_num8 PUBLIC ${NUM8LORA_INCLUDE_DIR} ${NUM8LORA_NUM8_ROOT})
    target_link_libraries(num8lora_receiver_num8 PUBLIC num8lora_static)

    target_compile_definitions(num8lora_adapter_config_test PRIVATE NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_adapter_config_test PRIVATE ${NUM8LORA_NUM8_ROOT})
    target_link_libraries(num8lora_adapter_config_test PRIVATE num8lora_receiver_num8)

    add_executable(num8lora_num8_test ${NUM8LORA_TEST_DIR}/num8lora_num8_test.c)
    target_compile_definitions(num8lora_num8_test PRIVATE NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_num8_test PRIVATE ${NUM8LORA_INCLUDE_DIR} ${NUM8LORA_NUM8_ROOT})
    target_link_libraries(num8lora_num8_test PRIVATE num8lora_static num8lora_receiver_num8)
    add_test(NAME num8lora_num8_test COMMAND num8lora_num8_test)

    add_executable(num8lora_flow_num8_test ${NUM8LORA_TEST_DIR}/num8lora_flow_num8_test.c)
    target_compile_definitions(num8lora_flow_num8_test PRIVATE NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_flow_num8_test PRIVATE ${NUM8LORA_INCLUDE_DIR} ${NUM8LORA_NUM8_ROOT})
    target_link_libraries(num8lora_flow_num8_test PRIVATE num8lora_static num8lora_receiver_num8)
    add_test(NAME num8lora_flow_num8_test COMMAND num8lora_flow_num8_test)
endif()
