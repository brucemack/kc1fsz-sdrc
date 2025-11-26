Releases
========

* v1 2025-11-22 - Installed at the W1TZK Repeate site.
* v1.1 2025-11-23 
    - Eliminated a lockout problem. Previously, any activity during the lockout
    would extend the lockout period. Now the lockout starts after timeout and
    will not be extended.
    - Raised the CW ID frequency to 600 Hz.
    

Dev Environment Setup Notes
===========================

Configuring Git on WSL:

    git config --global credential.helper "/mnt/c/Program\ Files/Git/mingw64/bin/git-credential-manager.exe"   

Building Notes
==============

Building host targets:

    cd kc1fsz-sdrc/sw
    git pull
    git submodule update --remote --recursive
    mkdir build
    cd build
    cmake -DHOST=ON ..
    make

Build PICO targets:

    export PICO_SDK_PATH=/home/bruce/pico/pico-sdk
    cd kc1fsz-sdrc/sw
    git pull
    git submodule update --remote --recursuve
    mkdir build
    cd build
    cmake -DPICO_BOARD=pico2 ..
    make main
    
Flashing
========

    ~/git/openocd/src/openocd -s ~/git/openocd/tcl -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "rp2350.dap.core1 cortex_m reset_config sysresetreq" -c "program main.elf verify reset exit"

Serial Console
==============

Connect serial-USB module to GPIO0/GPIO1 pins and use this command:

        minicom -b 115200 -c on -o -D /dev/ttyUSB0




