cmake_minimum_required(VERSION 3.16)
project(num8_lup C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

set(NUM8LORA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(NUM8LORA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NUM8LORA_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Legacy combined library
add_library(num8lora_static STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora.c
    ${NUM8LORA_SRC_DIR}/num8lora_runtime.c
    ${NUM8LORA_SRC_DIR}/num8lora_meta.c
)
target_include_directories(num8lora_static PUBLIC ${NUM8LORA_INCLUDE_DIR})

add_library(num8lora_shared SHARED
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora.c
    ${NUM8LORA_SRC_DIR}/num8lora_runtime.c
    ${NUM8LORA_SRC_DIR}/num8lora_meta.c
)
target_include_directories(num8lora_shared PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_compile_definitions(num8lora_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
set_target_properties(num8lora_shared PROPERTIES OUTPUT_NAME num8lora WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Async split protocol libraries
add_library(num8lora_sender_static STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_sender.c
)
target_include_directories(num8lora_sender_static PUBLIC ${NUM8LORA_INCLUDE_DIR})

add_library(num8lora_sender_shared SHARED
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_sender.c
)
target_include_directories(num8lora_sender_shared PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_compile_definitions(num8lora_sender_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
set_target_properties(num8lora_sender_shared PROPERTIES OUTPUT_NAME num8lora_sender WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(num8lora_receiver_static STATIC
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_receiver.c
    ${NUM8LORA_SRC_DIR}/num8lora_meta.c
)
target_include_directories(num8lora_receiver_static PUBLIC ${NUM8LORA_INCLUDE_DIR})

add_library(num8lora_receiver_shared SHARED
    ${NUM8LORA_SRC_DIR}/num8lora_op.c
    ${NUM8LORA_SRC_DIR}/num8lora_receiver.c
    ${NUM8LORA_SRC_DIR}/num8lora_meta.c
)
target_include_directories(num8lora_receiver_shared PUBLIC ${NUM8LORA_INCLUDE_DIR})
target_compile_definitions(num8lora_receiver_shared PRIVATE NUM8LORA_BUILD_SHARED=1)
set_target_properties(num8lora_receiver_shared PROPERTIES OUTPUT_NAME num8lora_receiver WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Tests
add_executable(num8lora_test ${NUM8LORA_TEST_DIR}/num8lora_test.c)
target_link_libraries(num8lora_test PRIVATE num8lora_static)

add_executable(num8lora_runtime_test ${NUM8LORA_TEST_DIR}/num8lora_runtime_test.c)
target_link_libraries(num8lora_runtime_test PRIVATE num8lora_static)

add_executable(num8lora_op_test ${NUM8LORA_TEST_DIR}/num8lora_op_test.c)
target_link_libraries(num8lora_op_test PRIVATE num8lora_sender_static)

add_executable(num8lora_async_test ${NUM8LORA_TEST_DIR}/num8lora_async_test.c)
target_link_libraries(num8lora_async_test PRIVATE num8lora_sender_static num8lora_receiver_static)

add_executable(num8lora_async_demo examples/async_demo.c)
target_link_libraries(num8lora_async_demo PRIVATE num8lora_sender_static num8lora_receiver_static)

enable_testing()
add_test(NAME num8lora_test COMMAND num8lora_test)
add_test(NAME num8lora_runtime_test COMMAND num8lora_runtime_test)
add_test(NAME num8lora_op_test COMMAND num8lora_op_test)
add_test(NAME num8lora_async_test COMMAND num8lora_async_test)

install(TARGETS
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

install(FILES
    ${NUM8LORA_INCLUDE_DIR}/num8lora.h
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

set(NUM8_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../NUM8-DENSE")
if(EXISTS "${NUM8_ENGINE_ROOT}/num8.c" AND EXISTS "${NUM8_ENGINE_ROOT}/num8.h")
    add_library(num8lora_num8_bridge STATIC ${NUM8LORA_SRC_DIR}/num8lora_num8_bridge.c)
    target_compile_definitions(num8lora_num8_bridge PUBLIC NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_num8_bridge PUBLIC ${NUM8LORA_INCLUDE_DIR} ${NUM8_ENGINE_ROOT})
    target_link_libraries(num8lora_num8_bridge PUBLIC num8lora_static)

    add_executable(num8lora_num8_test ${NUM8LORA_TEST_DIR}/num8lora_num8_test.c ${NUM8_ENGINE_ROOT}/num8.c)
    target_compile_definitions(num8lora_num8_test PRIVATE NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_num8_test PRIVATE ${NUM8LORA_INCLUDE_DIR} ${NUM8_ENGINE_ROOT})
    target_link_libraries(num8lora_num8_test PRIVATE num8lora_static num8lora_num8_bridge)
    add_test(NAME num8lora_num8_test COMMAND num8lora_num8_test)

    add_executable(num8lora_flow_num8_test ${NUM8LORA_TEST_DIR}/num8lora_flow_num8_test.c ${NUM8_ENGINE_ROOT}/num8.c)
    target_compile_definitions(num8lora_flow_num8_test PRIVATE NUM8LORA_ENABLE_NUM8=1)
    target_include_directories(num8lora_flow_num8_test PRIVATE ${NUM8LORA_INCLUDE_DIR} ${NUM8_ENGINE_ROOT})
    target_link_libraries(num8lora_flow_num8_test PRIVATE num8lora_static num8lora_num8_bridge)
    add_test(NAME num8lora_flow_num8_test COMMAND num8lora_flow_num8_test)
endif()
