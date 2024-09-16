To use the pico-sdk yout have to follow these steps:
1. Install prerequisites with `sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib`. The most important ones are
    - Cmake
    - gcc-arm-none-eabi
2. Download the sdk with `git clone https://github.com/raspberrypi/pico-sdk.git` to a known path
3. Add the `PICO_SDK_PATH` to our environment variables with `export PICO_SDK_PATH=<the path to the pico-sdk>`