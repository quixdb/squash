#!/bin/sh

# This script only exists to support Travis CI, since (AFAICT) it is
# not possible to perform different before_install steps depending on
# the operating system.  You should not be using this script.

case "${1}" in
    "deps")
        case "${TRAVIS_OS_NAME}" in
            "linux")
                sudo apt-get update -qq
                sudo apt-get install -qq python-software-properties
                sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
                sudo apt-get update -qq
                sudo apt-get install -qq \
                     git \
                     cmake \
                     build-essential \
                     libglib2.0-dev \
                     gcc-4.8 \
                     g++-4.8 \
                     gdb \
                     ragel
                sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
                sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20
                ;;
            "osx")
                brew update
                brew install \
                     ragel \
                     glib
                ;;
            *)
                echo "Unknown OS!"
                exit -1
                ;;
        esac
        ;;
    "build")
        COMMON_COMPILER_FLAGS="-Werror -fno-omit-frame-pointer"
        git submodule update --init --recursive
        mkdir build && cd build
        /bin/bash -x ../autogen.sh --disable-external CFLAGS="${COMMON_COMPILER_FLAGS}" CXXFLAGS="${COMMON_COMPILER_FLAGS}"
        make VERBOSE=1
        ;;
    "test")
        cd build

        case "${TRAVIS_OS_NAME}" in
            "linux")
                ulimit -c unlimited -S
                CTEST_OUTPUT_ON_FAILURE=TRUE make test || \
                    (for i in $(find ./ -maxdepth 1 -name 'core*' -print); do
                         gdb $(pwd)/tests core* -ex "thread apply all bt" -ex "set pagination 0" -batch;
                     done && exit -1)
                ;;
            "osx")
                CTEST_OUTPUT_ON_FAILURE=TRUE make test
                ;;
        esac
        ;;
    *)
        echo "Unknown step."
        exit -1
        ;;
esac
