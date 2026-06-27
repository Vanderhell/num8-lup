cmake_minimum_required(VERSION 3.16)
project(num8_lup VERSION 0.1.0 LANGUAGES C CXX)

if(WIN32 AND NOT DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0.22621.0 CACHE STRING "Windows SDK version" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(NUM8LORA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(NUM8LORA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NUM8LORA_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(NUM8LORA_PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/num8lora")
set(NUM8LORA_C_STANDARD 99 CACHE STRING "C language standard for C targets")

option(NUM8LORA_BUILD_TESTS "Build test executables and ctest entries" ON)
option(NUM8LORA_BUILD_EXAMPLES "Build example executables" ON)
option(NUM8LORA_BUILD_HOSTED_METADATA "Build hosted metadata persistence helpers" ON)
option(NUM8LORA_BUILD_NUM8_ADAPTER "Build the optional NUM8 adapter" OFF)
option(NUM8LORA_BUILD_STATIC_LIBS "Build static libraries" ON)
option(NUM8LORA_BUILD_SHARED_LIBS "Build shared libraries" ON)
set(NUM8LORA_NUM8_ROOT "" CACHE PATH "Path to the NUM8 dependency used by the optional adapter")

if(NOT NUM8LORA_BUILD_STATIC_LIBS AND NOT NUM8LORA_BUILD_SHARED_LIBS)
    message(FATAL_ERROR "At least one of NUM8LORA_BUILD_STATIC_LIBS or NUM8LORA_BUILD_SHARED_LIBS must be enabled")
endif()

function(num8lora_set_c99 target_name)
    set_target_properties(${target_name} PROPERTIES
        C_STANDARD ${NUM8LORA_C_STANDARD}
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)
endfunction()

function(num8lora_public_target target_name)
    target_include_directories(${target_name} PUBLIC
        $<BUILD_INTERFACE:${NUM8LORA_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    num8lora_set_c99(${target_name})
endfunction()

set(NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_metadata_core)

add_library(num8lora_metadata_core STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_codec.c
    ${NUM8LORA_SRC_DIR}/num8lora_metadata.c
)
num8lora_public_target(num8lora_metadata_core)
set_target_properties(num8lora_metadata_core PROPERTIES EXPORT_NAME metadata_core)

if(NUM8LORA_BUILD_STATIC_LIBS)
    add_library(num8lora_static STATIC
        ${NUM8LORA_SRC_DIR}/num8lora_codec.c
        ${NUM8LORA_SRC_DIR}/num8lora_op.c
        ${NUM8LORA_SRC_DIR}/num8lora.c
        ${NUM8LORA_SRC_DIR}/num8lora_runtime.c
    )
    num8lora_public_target(num8lora_static)
    target_link_libraries(num8lora_static PUBLIC num8lora_metadata_core)
    set_target_properties(num8lora_static PROPERTIES EXPORT_NAME legacy)
    list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_static)

    add_library(num8lora_sender_static STATIC
        ${NUM8LORA_SRC_DIR}/num8lora_codec.c
        ${NUM8LORA_SRC_DIR}/num8lora_op.c
        ${NUM8LORA_SRC_DIR}/num8lora_sender.c
    )
    num8lora_public_target(num8lora_sender_static)
    set_target_properties(num8lora_sender_static PROPERTIES EXPORT_NAME sender)
    list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_sender_static)

    add_library(num8lora_receiver_static STATIC
        ${NUM8LORA_SRC_DIR}/num8lora_codec.c
        ${NUM8LORA_SRC_DIR}/num8lora_op.c
        ${NUM8LORA_SRC_DIR}/num8lora_receiver.c
    )
    num8lora_public_target(num8lora_receiver_static)
    target_link_libraries(num8lora_receiver_static PUBLIC num8lora_metadata_core)
    set_target_properties(num8lora_receiver_static PROPERTIES EXPORT_NAME receiver)
    list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_receiver_static)

    if(NUM8LORA_BUILD_HOSTED_METADATA)
        add_library(num8lora_metadata_hosted STATIC
            ${NUM8LORA_SRC_DIR}/num8lora_meta.c
        )
        num8lora_public_target(num8lora_metadata_hosted)
        target_compile_definitions(num8lora_metadata_hosted PUBLIC NUM8LORA_BUILD_HOSTED_METADATA=1)
        target_link_libraries(num8lora_metadata_hosted PUBLIC num8lora_metadata_core)
        set_target_properties(num8lora_metadata_hosted PROPERTIES EXPORT_NAME metadata)
        list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_metadata_hosted)
    endif()

    if(NUM8LORA_BUILD_NUM8_ADAPTER)
        if(NOT EXISTS "${NUM8LORA_NUM8_ROOT}/num8.c" OR NOT EXISTS "${NUM8LORA_NUM8_ROOT}/num8.h")
            message(FATAL_ERROR "NUM8LORA_BUILD_NUM8_ADAPTER is ON but NUM8LORA_NUM8_ROOT does not contain num8.c and num8.h")
        endif()

        add_library(num8lora_receiver_num8 STATIC
            ${NUM8LORA_SRC_DIR}/num8lora_num8_bridge.c
            ${NUM8LORA_NUM8_ROOT}/num8.c
        )
        target_include_directories(num8lora_receiver_num8 PUBLIC
            $<BUILD_INTERFACE:${NUM8LORA_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${NUM8LORA_NUM8_ROOT}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
        num8lora_set_c99(num8lora_receiver_num8)
        target_compile_definitions(num8lora_receiver_num8 PUBLIC NUM8LORA_ENABLE_NUM8=1)
        target_link_libraries(num8lora_receiver_num8 PUBLIC num8lora_static)
        set_target_properties(num8lora_receiver_num8 PROPERTIES EXPORT_NAME receiver_num8)
        list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_receiver_num8)
    endif()
endif()

if(NUM8LORA_BUILD_SHARED_LIBS)
    add_library(num8lora_shared SHARED
        ${NUM8LORA_SRC_DIR}/num8lora_codec.c
        ${NUM8LORA_SRC_DIR}/num8lora_op.c
        ${NUM8LORA_SRC_DIR}/num8lora.c
        ${NUM8LORA_SRC_DIR}/num8lora_runtime.c
        ${NUM8LORA_SRC_DIR}/num8lora_metadata.c
    )
    num8lora_public_target(num8lora_shared)
    target_compile_definitions(num8lora_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
    set_target_properties(num8lora_shared PROPERTIES OUTPUT_NAME num8lora EXPORT_NAME legacy_shared WINDOWS_EXPORT_ALL_SYMBOLS ON)
    list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_shared)

    add_library(num8lora_sender_shared SHARED
        ${NUM8LORA_SRC_DIR}/num8lora_codec.c
        ${NUM8LORA_SRC_DIR}/num8lora_op.c
        ${NUM8LORA_SRC_DIR}/num8lora_sender.c
    )
    num8lora_public_target(num8lora_sender_shared)
    target_compile_definitions(num8lora_sender_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
    set_target_properties(num8lora_sender_shared PROPERTIES OUTPUT_NAME num8lora_sender EXPORT_NAME sender_shared WINDOWS_EXPORT_ALL_SYMBOLS ON)
    list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_sender_shared)

    add_library(num8lora_receiver_shared SHARED
        ${NUM8LORA_SRC_DIR}/num8lora_codec.c
        ${NUM8LORA_SRC_DIR}/num8lora_op.c
        ${NUM8LORA_SRC_DIR}/num8lora_receiver.c
        ${NUM8LORA_SRC_DIR}/num8lora_metadata.c
    )
    num8lora_public_target(num8lora_receiver_shared)
    target_compile_definitions(num8lora_receiver_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
    set_target_properties(num8lora_receiver_shared PROPERTIES OUTPUT_NAME num8lora_receiver EXPORT_NAME receiver_shared WINDOWS_EXPORT_ALL_SYMBOLS ON)
    list(APPEND NUM8LORA_INSTALL_EXPORT_TARGETS num8lora_receiver_shared)
endif()

if(NUM8LORA_BUILD_TESTS)
    add_executable(num8lora_test ${NUM8LORA_TEST_DIR}/num8lora_test.c)
    num8lora_public_target(num8lora_test)
    target_link_libraries(num8lora_test PRIVATE num8lora_static)
    target_compile_definitions(num8lora_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

    add_executable(num8lora_runtime_test ${NUM8LORA_TEST_DIR}/num8lora_runtime_test.c)
    num8lora_public_target(num8lora_runtime_test)
    target_link_libraries(num8lora_runtime_test PRIVATE num8lora_static)
    target_compile_definitions(num8lora_runtime_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

    add_executable(num8lora_op_test ${NUM8LORA_TEST_DIR}/num8lora_op_test.c)
    num8lora_public_target(num8lora_op_test)
    target_link_libraries(num8lora_op_test PRIVATE num8lora_sender_static)
    target_compile_definitions(num8lora_op_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

    add_executable(num8lora_async_test ${NUM8LORA_TEST_DIR}/num8lora_async_test.c)
    num8lora_public_target(num8lora_async_test)
    target_link_libraries(num8lora_async_test PRIVATE num8lora_sender_static num8lora_receiver_static)
    target_compile_definitions(num8lora_async_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

    add_executable(num8lora_metadata_test ${NUM8LORA_TEST_DIR}/num8lora_metadata_test.c)
    num8lora_public_target(num8lora_metadata_test)
    target_link_libraries(num8lora_metadata_test PRIVATE num8lora_metadata_core)
    target_compile_definitions(num8lora_metadata_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)
    if(NUM8LORA_BUILD_HOSTED_METADATA)
        target_link_libraries(num8lora_metadata_test PRIVATE num8lora_metadata_hosted)
    endif()

    add_executable(num8lora_adapter_config_test ${NUM8LORA_TEST_DIR}/num8lora_adapter_config_test.c)
    num8lora_public_target(num8lora_adapter_config_test)
    target_link_libraries(num8lora_adapter_config_test PRIVATE num8lora_static)
    target_compile_definitions(num8lora_adapter_config_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

    add_executable(num8lora_codec_contract_test ${NUM8LORA_TEST_DIR}/num8lora_codec_contract_test.c)
    num8lora_public_target(num8lora_codec_contract_test)
    target_link_libraries(num8lora_codec_contract_test PRIVATE num8lora_static num8lora_sender_static)
    target_compile_definitions(num8lora_codec_contract_test PRIVATE
        NUM8LORA_BUILD_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
        NUM8LORA_BUILD_NUM8_ADAPTER=$<IF:$<BOOL:${NUM8LORA_BUILD_NUM8_ADAPTER}>,1,0>)

    add_executable(num8lora_header_smoke_num8lora ${NUM8LORA_TEST_DIR}/header_smoke_num8lora.c)
    num8lora_public_target(num8lora_header_smoke_num8lora)
    add_executable(num8lora_header_smoke_num8lora_op ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_op.c)
    num8lora_public_target(num8lora_header_smoke_num8lora_op)
    add_executable(num8lora_header_smoke_num8lora_metadata ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_metadata.c)
    num8lora_public_target(num8lora_header_smoke_num8lora_metadata)
    add_executable(num8lora_header_smoke_num8lora_sender ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_sender.c)
    num8lora_public_target(num8lora_header_smoke_num8lora_sender)
    target_link_libraries(num8lora_header_smoke_num8lora_sender PRIVATE num8lora_sender_static)
    add_executable(num8lora_header_smoke_num8lora_receiver ${NUM8LORA_TEST_DIR}/header_smoke_num8lora_receiver.c)
    num8lora_public_target(num8lora_header_smoke_num8lora_receiver)
    target_link_libraries(num8lora_header_smoke_num8lora_receiver PRIVATE num8lora_receiver_static)
    add_executable(num8lora_header_smoke_repeat ${NUM8LORA_TEST_DIR}/header_smoke_repeat.c)
    num8lora_public_target(num8lora_header_smoke_repeat)
    add_executable(num8lora_header_smoke_cpp ${NUM8LORA_TEST_DIR}/header_smoke_cpp.cpp)
    target_include_directories(num8lora_header_smoke_cpp PRIVATE ${NUM8LORA_INCLUDE_DIR})
    add_executable(num8lora_header_smoke_mtu
        ${NUM8LORA_TEST_DIR}/header_smoke_mtu_a.c
        ${NUM8LORA_TEST_DIR}/header_smoke_mtu_b.c)
    num8lora_public_target(num8lora_header_smoke_mtu)
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

    add_test(
        NAME num8lora_package_install_consumer
        COMMAND ${CMAKE_COMMAND}
            -DNUM8LORA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DNUM8LORA_TEST_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/package-consumer-check
            -DNUM8LORA_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DNUM8LORA_EXPECT_HOSTED_METADATA=$<IF:$<BOOL:${NUM8LORA_BUILD_HOSTED_METADATA}>,1,0>
            -P ${NUM8LORA_TEST_DIR}/check_package_install_consumer.cmake)
endif()

if(NUM8LORA_BUILD_EXAMPLES)
    add_executable(num8lora_async_demo examples/async_demo.c)
    num8lora_public_target(num8lora_async_demo)
    target_link_libraries(num8lora_async_demo PRIVATE num8lora_sender_static num8lora_receiver_static)
endif()

set(NUM8LORA_CONFIG_INSTALL_DIR "${NUM8LORA_PACKAGE_DIR}")

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/num8loraConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/num8loraConfig.cmake
    INSTALL_DESTINATION ${NUM8LORA_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/num8loraConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    TARGETS ${NUM8LORA_INSTALL_EXPORT_TARGETS}
    EXPORT num8loraTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT num8loraTargets
    FILE num8loraTargets.cmake
    NAMESPACE num8lora::
    DESTINATION ${NUM8LORA_CONFIG_INSTALL_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/num8loraConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/num8loraConfigVersion.cmake
    DESTINATION ${NUM8LORA_CONFIG_INSTALL_DIR}
)

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
