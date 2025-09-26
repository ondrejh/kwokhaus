cmake_minimum_required(VERSION 3.13)

# Define the project name (once)
set(PROJECT_NAME proj)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/ext/pico-sdk)
include(pico_sdk_import.cmake)

# Set project name
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(${PROJECT_NAME}
  src/main.c
)

# add pico_stdlib library which aggregates commonly used features
target_link_libraries(${PROJECT_NAME}
  pico_stdlib
)

# Enable USB output, disable UART if desired
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
