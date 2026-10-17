cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# Предупреждения компилятора должны прийти и в библиотеки
add_compile_options(-Wall)

# 1. Железо прибора: единственное место проекта с SDK
add_library(platform STATIC
    platform/platform.c
)
target_include_directories(platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(platform PRIVATE
    pico_stdlib
)

# Выключатель профилирования: 1 — включено, 0 — выключено
# Позволяет переопределить значение при сборке: cmake -DPROFILING_ENABLED=0 ..
if(NOT DEFINED PROFILING_ENABLED)
    set(PROFILING_ENABLED 1)
endif()
target_compile_definitions(profiling PUBLIC PROFILING_ENABLED=${PROFILING_ENABLED})

# 2. Логика прибора: задачи и команды, СТРОГО БЕЗ SDK
add_library(logic STATIC
    logic/led-task/led-task.c
    logic/button-task/button-task.c
    logic/pi-task/pi-task.c
    logic/api/api.c
    logic/api/api-callbacks.c
)

target_include_directories(logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/api
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/led-task
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/button-task
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/pi-task
)

target_link_libraries(logic PUBLIC
    stdio_text_protocol
)

target_link_libraries(logic PRIVATE
    device
    logging
    profiling
    systime
)

# 3. Исполняемый файл проекта
add_executable(${PROJECT_NAME}
    main.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    platform
    logic
    device
    logging
    stdio_text_protocol
    profiling
    systime
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/api
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/led-task
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/button-task
    ${CMAKE_CURRENT_SOURCE_DIR}/logic/pi-task
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})