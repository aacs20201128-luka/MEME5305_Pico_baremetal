# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(260818 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

#memory.c 沒有被你的 CMake 專案列入編譯目標，因此 VS Code IntelliSense 不知道該用 Pico 專案的哪套編譯設定分析它。
# BH1750 - Pico SDK API
#add_executable(260818 bh1750_api.c)
# BH1750 - Bare-Metal / Register-Level
#add_executable(260818 bh1750_baremetal.c)
# UART - Pico SDK API
#add_executable(260818 uart_api.c)
# UART - Bare-Metal / Register-Level
#add_executable(260818 uart_baremetal.c)
# BH1750 I2C + UART TX - Bare-Metal 整合版
add_executable(260818 bh1750_uart_baremetal.c)
# GPIO Voltage Test
#add_executable(260818 gpio_voltage_test.c)

pico_set_program_name(260818 "260818")
pico_set_program_version(260818 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(260818 0)
pico_enable_stdio_usb(260818 1)

# Add the standard library to the build
target_link_libraries(260818
        pico_stdlib
        hardware_i2c
        hardware_uart #UART用
        )

# Add the standard include files to the build
target_include_directories(260818 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(260818 
        
        )

pico_add_extra_outputs(260818)

