OLD_DIRECTORY="$(realpath "$pwd")"
cd "$(realpath "$0")" # change to the root of the rp2040 directory

if [[ "$OSTYPE" =~ ^darwin ]]; then
    # https://wellys.com/posts/rp2040_c_macos/
    brew install cmake make
    # Both Arm (M-variants) and Intel Mac may use this command
    brew install arm-none-eabi-gcc
    # various tools required for picotool etc, install as well
    # brew install libtool automake libusb wget pkg-config gcc texinfo

    # get gnu toolchain
    # GNU_TOOLCHAIN_LINK="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2?rev=58ed196feb7b4ada8288ea521fa87ad5&hash=F095D2C3D1659B531BE482826BE4A6E1"
    # curl $GNU_TOOLCHAIN_LINK --output gnu_toolchain.tar.bz2
    # tar -xf gnu_toolchain.tar.bz2 --directory
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
fi


cd $OLD_DIRECTORY
