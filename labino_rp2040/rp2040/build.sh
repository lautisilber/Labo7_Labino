OLD_DIRECTORY="$(realpath "$pwd")"
SCRIPT_PATH="$(realpath "$0")"
ROOT_PATH="$(dirname "$SCRIPT_PATH")" # get this script's full path

export PICO_SDK_PATH="$ROOT_PATH/pico-sdk"

BUILD_DIR="$ROOT_PATH/build"
if [ ! -d "$BUILD_DIR" ];
then
    mkdir "$BUILD_DIR"
fi
cd "$BUILD_DIR"
cmake ../
make -j4
cd "$OLD_DIRECTORY"
