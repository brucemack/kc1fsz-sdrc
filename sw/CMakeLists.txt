cmake_minimum_required(VERSION 3.13)
# Order matters here, need to pull in pico_sdk_import.cmake
if (HOST)
# Nothing
else()
include(pico_sdk_import.cmake)
endif()

project(kc1fsz-sdrc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(HOME $ENV{HOME})

# Order matters here, need to call pico_sdk_init()
if (HOST)
# Nothing
else()
pico_sdk_init()
endif()

# ===== HOST EXECUTABLES =====================================================

if (HOST)

add_executable(main
  src/test/test-AudioCore.cpp
  src/AudioCore.cpp
  src/DTMFDetector2.cpp
  #src/TxControl.cpp
  #src/TestToneGenerator.cpp
  #src/Config.cpp
  #src/StdRx.cpp
  gsm-0610-codec/src/fixed_math.cpp
  radlib/util/dsp_util.cpp    
  cmsis-dsp-mock/src/main.cpp
)
target_include_directories(main PRIVATE
  src
  cmsis-dsp-mock/include
  radlib
  kc1fsz-tools-cpp/include
)

add_executable(dtmf-test-1
  src/test/dtmf-test-1.cpp
  src/DTMFDetector2.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  gsm-0610-codec/src/fixed_math.cpp
) 
target_include_directories(dtmf-test-1 PRIVATE
  src
  cmsis-dsp-mock/include
  radlib
  kc1fsz-tools-cpp/include
)
target_compile_options(dtmf-test-1 PRIVATE -fstack-protector-all -Wall -Wpedantic -g)
target_include_directories(dtmf-test-1 PRIVATE kc1fsz-tools-cpp/include)

# ===== PICO EXECUTABLES =====================================================
else()

add_executable(test1
  src/test/test-AudioCore-pico.cpp
  src/AudioCore.cpp
  radlib/util/dsp_util.cpp  
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/rp2040/PicoPerfTimer.cpp
)
target_compile_definitions(test1 PRIVATE -DPICO_BUILD=1)
pico_enable_stdio_usb(test1 0)
pico_enable_stdio_uart(test1 1)
pico_generate_pio_header(test1 ${CMAKE_CURRENT_LIST_DIR}/src/i2s.pio)

target_include_directories(test1 PRIVATE 
  src
  kc1fsz-tools-cpp/include
  radlib
  ${HOME}/pico/CMSISDSP/CMSIS-DSP/Include
  ${HOME}/pico/CMSISDSP/CMSIS_6/CMSIS/Core/Include
)

target_link_libraries(test1 
  pico_stdlib hardware_i2c hardware_clocks hardware_pwm hardware_dma hardware_pio 
  hardware_flash
  ${HOME}/pico/CMSISDSP/build/bin_dsp/libCMSISDSP.a
)

target_compile_options(test1 PRIVATE -g)

# Set target properties for UF2 build
pico_add_extra_outputs(test1)

# ------ main ----------------------------------------------------------------
# This is the main controller project

add_executable(main
  src/i2s.pio
  src/main.cpp
  src/AudioCore.cpp
  src/Config.cpp
  src/ShellCommand.cpp
  src/Tx.cpp
  src/Rx.cpp
  src/StdTx.cpp
  src/StdRx.cpp
  src/TxControl.cpp
  src/CourtesyToneGenerator.cpp
  src/IDToneGenerator.cpp
  src/TestToneGenerator.cpp
  #src/VoiceGenerator.cpp
  radlib/util/dsp_util.cpp  
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/CommandShell.cpp
  kc1fsz-tools-cpp/src/WindowAverage.cpp
  kc1fsz-tools-cpp/src/rp2040/PicoPollTimer.cpp
  kc1fsz-tools-cpp/src/rp2040/PicoPerfTimer.cpp
)
target_compile_definitions(main PRIVATE -DPICO_BUILD=1)
pico_enable_stdio_usb(main 0)
pico_enable_stdio_uart(main 1)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/src/i2s.pio)

target_include_directories(main PRIVATE 
  src
  kc1fsz-tools-cpp/include
  radlib
  ${HOME}/pico/CMSISDSP/CMSIS-DSP/Include
  ${HOME}/pico/CMSISDSP/CMSIS_6/CMSIS/Core/Include
)

target_link_libraries(main 
  pico_stdlib hardware_i2c hardware_clocks hardware_pwm hardware_dma hardware_pio 
  hardware_flash
  ${HOME}/pico/CMSISDSP/build/bin_dsp/libCMSISDSP.a
)

target_compile_options(main PRIVATE -g)

# Set target properties for UF2 build
pico_add_extra_outputs(main)


endif()
