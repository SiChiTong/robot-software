#!/usr/bin/env bash

# Make the script fail if any command in it fail
set -e

if [ "$BUILD_TYPE" == "" ]
then
    echo "Please define \$BUILD_TYPE!"
    exit 1
fi

source env/bin/activate
PROJPATH=$(pwd)
export PATH=$PROJPATH/gcc-arm-none-eabi-4_9-2014q4/bin/:$PATH

export CFLAGS="$CFLAGS -I $HOME/cpputest/include/"
export CXXFLAGS="$CXXFLAGS -I $HOME/cpputest/include/"
export LDFLAGS="$CXXFLAGS -L $HOME/cpputest/lib/"

ROBOT_FLAG=""
if [ -n "$ROBOT" ]
then
    ROBOT_FLAG="ROBOT=$ROBOT"
fi


case $BUILD_TYPE in
    tests)
        pushd master-firmware
        packager
        mkdir build
        cd build
        cmake ..
        make check
        popd

        pushd motor-control-firmware
        packager
        mkdir build
        cd build
        cmake ..
        make check
        popd
        ;;

    build)
        echo "build $PLATFORM"
        pushd $PLATFORM
        packager
        make $ROBOT_FLAG dsdlc
        make $ROBOT_FLAG
        popd
        ;;
    *)
        echo "Unknown build type $BUILD_TYPE"
        exit 1
        ;;
esac
