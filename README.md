Configuring Git on WSL:

    git config --global credential.helper "/mnt/c/Program\ Files/Git/mingw64/bin/git-credential-manager.exe"   

Building Notes
==============

Building host targets:

    mkdir build
    cd build
    cmake -DHOST=ON ..
    make

Build PICO targets:

    export PICO_SDK_PATH=/home/bruce/pico/pico-sdk
    git submodule update --init
    mkdir build
    cd build
    cmake -DPICO_BOARD=pico2 ..
    




