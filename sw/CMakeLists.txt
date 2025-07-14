cmake_minimum_required(VERSION 3.13)
project(kc1fsz-sdrc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

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

# ------ dtmf-test-1 ---------------------------------------------------------
# Target: Host

add_executable(dtmf-test-1
  src/test/dtmf-test-1.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/DTMFDetector.cpp
  gsm-0610-codec/src/fixed_math.cpp
) 
target_compile_options(dtmf-test-1 PRIVATE -fstack-protector-all -Wall -Wpedantic -g)
target_include_directories(dtmf-test-1 PRIVATE kc1fsz-tools-cpp/include)
