export PICO_SDK_PATH=/Users/lautarosilbergleit/dev/rpi_pico/pico-sdk
export PICO_TOOLCHAIN_PATH=/Applications/ArmGNUToolchain/12.2.rel1/arm-none-eabi

BUILD_DIR="build"
if [ ! -d $BUILD_DIR ];
then
    mkdir $BUILD_DIR
fi
cd $BUILD_DIR
cmake ../
make -j4
cd ..
