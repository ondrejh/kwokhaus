cmake_minimum_required(VERSION 3.13)

# Define the project name (once)
set(PROJECT_NAME proj)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../ext/pico-sdk)
include(pico_sdk_import.cmake)

# Set project name
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Gather all U8g2 .c files
file(GLOB U8G2_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/../ext/u8g2/csrc/u8g2_*.c
  ${CMAKE_CURRENT_LIST_DIR}/../ext/u8g2/csrc/u8x8_*.c
)

# rest of your project
add_executable(${PROJECT_NAME}
  main.c
  display.c
  ws2812.c
  ws2812.pio
  ${U8G2_SOURCES}
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/../ext/pico-examples/pio/ws2812/ws2812.pio)

# include U8g2 headers
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../ext/u8g2/csrc
)

# add pico_stdlib library which aggregates commonly used features
target_link_libraries(${PROJECT_NAME}
  pico_stdlib
  hardware_pio
  hardware_i2c
)

# Enable USB output, disable UART if desired
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
