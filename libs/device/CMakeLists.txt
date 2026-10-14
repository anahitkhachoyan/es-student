add_library(device STATIC
    src/device.c
)

target_include_directories(device PUBLIC
    include
)

target_link_libraries(device PRIVATE pico_stdlib)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)