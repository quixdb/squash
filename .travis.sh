#!/bin/sh

# This script only exists to support Travis CI, since (AFAICT) it is
# not possible to perform different before_install steps depending on
# the operating system.  You should not be using this script.

CC=gcc-4.8
CXX=g++-4.8

case "${COMPILER}" in
    "clang")
        CC=clang
        CXX=clang++
        ;;
    "gcc-5")
        CC=gcc-5
        CXX=g++-5
        ;;
    "gcc-4.8")
        CC=gcc-4.8
        CXX=g++-4.8
        ;;
    "gcc-4.6")
        CC=gcc-4.6
        CXX=g++-4.6
        ;;
esac

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
                     gcc-5 \
                     g++-5 \
                     gdb \
                     ragel
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
        case "${BUILD_TYPE}" in
            "asan")
                COMMON_COMPILER_FLAGS="${COMMON_COMPILER_FLAGS} -fsanitize=address"
                ;;
            "tsan")
                COMMON_COMPILER_FLAGS="${COMMON_COMPILER_FLAGS} -fsanitize=thread"
                ;;
            "ubsan")
                COMMON_COMPILER_FLAGS="${COMMON_COMPILER_FLAGS} -fsanitize=undefined"
                ;;
        esac

        CONFIGURE_FLAGS="--disable-external"
        case "${BUILD_TYPE}" in
            "coverage")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} --enable-coverage"
                ;;
        esac

        git submodule update --init --recursive
        mkdir build && cd build
        /bin/bash -x ../autogen.sh CC="${CC}" CXX="${CXX}" ${CONFIGURE_FLAGS} CFLAGS="${COMMON_COMPILER_FLAGS}" CXXFLAGS="${COMMON_COMPILER_FLAGS}"
        make VERBOSE=1

        case "${BUILD_TYPE}" in
            "coverage")
                make coverage
                ;;
        esac
        ;;
    "test")
        cd build

        case "${TRAVIS_OS_NAME}" in
            "linux")
                ulimit -c unlimited
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
