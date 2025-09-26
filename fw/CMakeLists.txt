# Vyžadujeme minimální CMake verzi
cmake_minimum_required(VERSION 3.13)

# Nastavení toolchain přes Pico SDK
set(PICO_SDK_PATH "/home/hejdovi/Projekty/kwokhaus/fw/ext/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Explicitně říkáme CMake, že budeme cross-compilovat pro RP2040
#set(CMAKE_SYSTEM_NAME Generic)
#set(CMAKE_SYSTEM_PROCESSOR arm)


# Nastavení ASM kompilátoru pro RP2040
#enable_language(ASM)
#set(CMAKE_ASM_COMPILER /usr/bin/arm-none-eabi-gcc)


# Define the project name (once)
set(PROJECT_NAME trigger)

project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializace Pico SDK
pico_sdk_init()

# Vytvoření hlavního cíle
add_executable(${PROJECT_NAME}
    src/main.c
    # další C soubory, pokud jsou
)

# Přidání knihoven z Pico SDK (volitelné, podle projektu)
target_link_libraries(${PROJECT_NAME} pico_stdlib)

# Enable USB output, disable UART if desired
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Nastavení výstupu pro .elf, .bin a .hex
pico_add_extra_outputs(${PROJECT_NAME})
