#!/bin/sh

# This script only exists to support Travis CI.  You should not be
# using this script.

export GCOV=gcov

CROSS_COMPILE=no
case "${CC}" in
    "clang")
        export CC=clang
        export CXX=clang++
        ;;
    "clang-"*)
        export CC=clang-${CC#*-}
        export CXX=clang++-${CC#*-}
        ;;
    "gcc-"*)
        export CC=gcc-${CC#*-}
        export CXX=g++-${CC#*-}
        export GCOV=gcov-${CC#*-}
        ;;
    "x86_64-w64-mingw32-gcc")
        export CC=x86_64-w64-mingw32-gcc
        export CXX=x86_64-w64-mingw32-g++
        export GCOV=x86_64-w64-mingw32-gcov
        export RC_COMPILER=x86_64-w64-mingw32-windres
        CROSS_COMPILE=yes
        ;;
    "i686-w64-mingw32-gcc")
        export CC=i686-w64-mingw32-gcc
        export CXX=i686-w64-mingw32-g++
        export GCOV=i686-w64-mingw32-gcov
        export RC_COMPILER=i686-w64-mingw32-windres
        CROSS_COMPILE=yes
        ;;
    "icc")
        # ICC doesn't work from pull requests because encrypted
        # variables (which are used for the serial number) aren't
        # available to pull requests, so unfortunately we need to skip
        # it.
        if  [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
            export CC=icc
            export CXX=icpc
            export GCOV=gcov
        else
            exit 0;
        fi
        ;;
    *)
        CC="gcc"
        export CC=gcc
        export CXX=g++
        export GCOV=gcov
        ;;
esac

if [ "${TRAVIS_BRANCH}" = "coverity" -a "${TRAVIS_OS_NAME}" = "linux" -a "${BUILD_TYPE}" = "debug" -a "${CC}" = "gcc-6" ]; then
    BUILD_TYPE=coverity
    COVERITY_SCAN_PROJECT_NAME="quixdb/squash"
    COVERITY_TOOL_BASE="/tmp/coverity-scan-analysis"
fi

case "${1}" in
    "configure")
        COMMON_COMPILER_FLAGS="-Werror -fno-omit-frame-pointer -fstack-protector-all -D_FORTIFY_SOURCE=2"
        case "${BUILD_TYPE}" in
            "asan")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DENABLE_SANITIZER=address"
                ;;
            "tsan")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DENABLE_SANITIZER=thread"
                ;;
            "ubsan")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DENABLE_SANITIZER=undefined"
                ;;
        esac

        CONFIGURE_FLAGS="-DFORCE_IN_TREE_DEPENDENCIES=yes"
        if [ "${BUILD_TYPE}" = "release" ]; then
            CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DCMAKE_BUILD_TYPE=Release"
        else
            CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DCMAKE_BUILD_TYPE=Debug"
        fi

        case "${BUILD_TYPE}" in
            "coverage")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DENABLE_COVERAGE=yes"
                ;;
            "asan")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DENABLE_DENSITY=no"
                ;;
        esac

        case "${CC}" in
            "i686-w64-mingw32-gcc")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DCMAKE_FIND_ROOT_PATH=/usr/i686-w64-mingw32"
                ;;
            "x86_64-w64-mingw32-gcc")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32"
                ;;
        esac

        if [ "${CROSS_COMPILE}" = "yes" ]; then
            CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DCMAKE_SYSTEM_NAME=Windows -DDISABLE_UNIT_TESTS=yes -DENABLE_GIPFELI=no"
        fi

        case "${TRAVIS_OS_NAME}" in
            "osx")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} -DENABLE_MS_COMPRESS=no -DENABLE_NCOMPRESS=no -DENABLE_LZHAM=no"
                # See https://github.com/ebiggers/libdeflate/issues/5#issuecomment-177718351
                case "${CC}" in
                    "gcc-"*)
                        COMMON_COMPILER_FLAGS="${COMMON_COMPILER_FLAGS} -Wa,-q"
                        ;;
                esac
                ;;
        esac

        git submodule update --init --recursive
        echo "cmake . ${CONFIGURE_FLAGS} -DCMAKE_C_FLAGS=\"${COMMON_COMPILER_FLAGS}\" -DCMAKE_CXX_FLAGS=\"${COMMON_COMPILER_FLAGS}\""
        cmake . ${CONFIGURE_FLAGS} -DCMAKE_C_FLAGS="${COMMON_COMPILER_FLAGS}" -DCMAKE_CXX_FLAGS="${COMMON_COMPILER_FLAGS}"
        ;;

    "make")
        make VERBOSE=1
        ;;

    "test")
        if [ "x${CROSS_COMPILE}" != "xyes" ]; then
            ./tests/test-squash || exit 1
        fi

        case "${BUILD_TYPE}" in
            "coverage")
                coveralls --gcov "${GCOV}" -e CMakeFiles -e plugins -e examples -e tests -e squash/tinycthread
                ;;
        esac
        ;;
    *)
        echo "Unknown step."
        exit 1
        ;;
esac
