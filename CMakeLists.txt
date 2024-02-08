cmake_minimum_required(VERSION 3.9)

include(pico_sdk_import.cmake)

project(gaggico)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(gaggico
  src/network/discovery.cpp
  src/network/network.cpp
  src/panic.cpp
  src/hardware/hardware.cpp
  src/control/control.cpp
  src/control/protocol.cpp
  src/network/messages.cpp
  src/network/ntp.cpp
  src/settings.cpp
  src/main.cpp)

pico_generate_pio_header(gaggico ${CMAKE_CURRENT_LIST_DIR}/src/hardware/pac.pio)

target_include_directories(gaggico PRIVATE
  src/network
  src)

target_link_libraries(gaggico
  pico_stdlib
  pico_multicore
  pico_cyw43_arch_lwip_threadsafe_background
  pico_lwip_sntp
  hardware_spi
  hardware_adc)

string(TIMESTAMP CURRENT_YEAR "%Y")

target_compile_definitions(gaggico PRIVATE CURRENT_YEAR=${CURRENT_YEAR})
# target_compile_definitions(gaggico PRIVATE DEBUG_NO_HARDWARE)

pico_enable_stdio_usb(gaggico 1)
pico_enable_stdio_uart(gaggico 1)

pico_add_extra_outputs(gaggico)
