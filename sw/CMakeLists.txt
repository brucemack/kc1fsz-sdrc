cmake_minimum_required(VERSION 3.13)
project(kc1fsz-sdrc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(HOME $ENV{HOME})

# ===== HOST EXECUTABLES =====================================================

if (HOST)

add_executable(main
  src/test/test-AudioCore.cpp
  src/AudioCore.cpp
  radlib/util/dsp_util.cpp  
  cmsis-dsp-mock/src/main.cpp
)
target_include_directories(main PRIVATE
  cmsis-dsp-mock/include
  radlib
  src
)

add_executable(dtmf-test-1
  src/test/dtmf-test-1.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/DTMFDetector.cpp
  gsm-0610-codec/src/fixed_math.cpp
) 
target_compile_options(dtmf-test-1 PRIVATE -fstack-protector-all -Wall -Wpedantic -g)
target_include_directories(dtmf-test-1 PRIVATE kc1fsz-tools-cpp/include)

# ===== PICO EXECUTABLES =====================================================
else()

include(pico_sdk_import.cmake)
pico_sdk_init()

add_executable(main
  src/test/test-AudioCore-pico.cpp
  #src/AudioCore.cpp
  #radlib/util/dsp_util.cpp  
#  src/i2s.pio
#  src/main.cpp
#  src/Config.cpp
#  src/ShellCommand.cpp
#  src/Tx.cpp
#  src/TxControl.cpp
#  src/Rx.cpp
#  src/CourtesyToneGenerator.cpp
#  src/IDToneGenerator.cpp
#  src/VoiceGenerator.cpp
#  src/test/TestTx.cpp
#  src/test/TestRx.cpp
#  src/hw/StdTx.cpp
#  src/hw/StdRx.cpp
#  kc1fsz-tools-cpp/src/Common.cpp
#  kc1fsz-tools-cpp/src/CommandShell.cpp
#  kc1fsz-tools-cpp/src/ToneSynthesizer.cpp
#  kc1fsz-tools-cpp/src/rp2040/PicoPollTimer.cpp
#  kc1fsz-tools-cpp/src/rp2040/PicoPerfTimer.cpp
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

# Set target properties for UF2 build
pico_add_extra_outputs(main)

endif()
