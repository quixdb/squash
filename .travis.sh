#!/bin/sh

# This script only exists to support Travis CI, since (AFAICT) it is
# not possible to perform different before_install steps depending on
# the operating system.  You should not be using this script.

export GCOV=gcov

CROSS_COMPILE=no
case "${COMPILER}" in
    "clang")
        export CC=clang
        export CXX=clang++
        ;;
    "clang-3.4")
        export CC=clang
        export CXX=clang++
        ;;
    "clang-3.5")
        export CC=clang-3.5
        export CXX=clang++-3.5
        ;;
    "clang-3.6")
        export CC=clang-3.6
        export CXX=clang++-3.6
        ;;
    "clang-3.7")
        export CC=clang-3.7
        export CXX=clang++-3.7
        ;;
    "gcc-4.6")
        export CC=gcc-4.6
        export CXX=g++-4.6
        export GCOV=gcov-4.6
        ;;
    "gcc-4.8")
        export CC=gcc-4.8
        export CXX=g++-4.8
        export GCOV=gcov-4.8
        ;;
    "gcc-5")
        export CC=gcc-5
        export CXX=g++-5
        export GCOV=gcov-5
        ;;
    "x86_64-w64-mingw32-gcc")
        export CC=x86_64-w64-mingw32-gcc
        export CXX=x86_64-w64-mingw32-g++
        export GCOV=x86_64-w64-mingw32-gcov
        CROSS_COMPILE=yes
        ;;
    "i686-w64-mingw32-gcc")
        export CC=i686-w64-mingw32-gcc
        export CXX=i686-w64-mingw32-g++
        export GCOV=i686-w64-mingw32-gcov
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
        COMPILER="gcc-5"
        export CC=gcc-5
        export CXX=g++-5
        export GCOV=gcov-5
        ;;
esac

if [ "${TRAVIS_BRANCH}" = "coverity" -a "${TRAVIS_OS_NAME}" = "linux" -a "${BUILD_TYPE}" = "debug" -a "${COMPILER}" = "gcc-5" ]; then
    BUILD_TYPE=coverity
    COVERITY_SCAN_PROJECT_NAME="quixdb/squash"
    COVERITY_TOOL_BASE="/tmp/coverity-scan-analysis"
fi

case "${1}" in
    "deps")
        case "${TRAVIS_OS_NAME}" in
            "linux")
                . /etc/lsb-release

                sudo apt-get update -qq
                sudo apt-get install -qq python-software-properties
                sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test

                sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys AF4F7421
                for CLANG_VERSION in 3.4 3.5 3.6 3.7; do
                    sudo apt-add-repository -y \
                         "deb http://llvm.org/apt/${DISTRIB_CODENAME}/ llvm-toolchain-${DISTRIB_CODENAME}-${CLANG_VERSION} main"
                done

                sudo apt-get update -qq
                sudo apt-get install -qq \
                     cmake \
                     build-essential \
                     libglib2.0-dev \
                     gdb \
                     ragel

                case "${COMPILER}" in
                    "gcc-5")
                        sudo apt-get install -qq gcc-5 g++-5
                        ;;
                    "gcc-4.8")
                        sudo apt-get install -qq gcc-4.8 g++-4.8
                        ;;
                    "clang-3.4")
                        sudo apt-get install -qq clang-3.4
                        ;;
                    "clang-3.5")
                        sudo apt-get install -qq clang-3.5
                        ;;
                    "clang-3.6")
                        sudo apt-get install -qq clang-3.6
                        ;;
                    "clang-3.7")
                        sudo apt-get install -qq clang-3.7
                        ;;
                    "x86_64-w64-mingw32-gcc")
                        sudo apt-get install -qq mingw-w64
                        ;;
                    "i686-w64-mingw32-gcc")
                        sudo apt-get install -qq mingw-w64
                        ;;
                    "icc")
                        wget -q -O /dev/stdout \
                             'https://raw.githubusercontent.com/nemequ/icc-travis/master/install-icc.sh' | \
                            /bin/sh
                        ;;
                esac

                ${CC} --version

                case "${BUILD_TYPE}" in
                    "coverage")
                        sudo apt-get install -qq python-pip lcov
                        sudo pip install cpp-coveralls
                        ;;
                    "coverity")
                        PLATFORM=`uname`
                        TOOL_ARCHIVE=/tmp/cov-analysis-${PLATFORM}.tgz
                        TOOL_URL=https://scan.coverity.com/download/${PLATFORM}
                        SCAN_URL="https://scan.coverity.com"

                        # Verify upload is permitted
                        AUTH_RES=`curl -s --form project="$COVERITY_SCAN_PROJECT_NAME" --form token="$COVERITY_SCAN_TOKEN" $SCAN_URL/api/upload_permitted`
                        if [ "$AUTH_RES" = "Access denied" ]; then
                            curl -s "http://code.coeusgroup.com/test?name=${COVERITY_SCAN_PROJECT_NAME}&token=${COVERITY_SCAN_TOKEN}"
                            echo -e "\033[33;1mCoverity Scan API access denied. Check COVERITY_SCAN_PROJECT_NAME and COVERITY_SCAN_TOKEN.\033[0m"
                            exit 1
                        else
                            AUTH=`echo $AUTH_RES | ruby -e "require 'rubygems'; require 'json'; puts JSON[STDIN.read]['upload_permitted']"`
                            if [ "$AUTH" = "true" ]; then
                                echo -e "\033[33;1mCoverity Scan analysis authorized per quota.\033[0m"
                            else
                                WHEN=`echo $AUTH_RES | ruby -e "require 'rubygems'; require 'json'; puts JSON[STDIN.read]['next_upload_permitted_at']"`
                                echo -e "\033[33;1mCoverity Scan analysis NOT authorized until $WHEN.\033[0m"
                                exit 1
                            fi
                        fi

                        if [ ! -d $COVERITY_TOOL_BASE ]; then
                            # Download Coverity Scan Analysis Tool
                            if [ ! -e $TOOL_ARCHIVE ]; then
                                echo "\033[33;1mDownloading Coverity Scan Analysis Tool...\033[0m"
                                wget -nv -O $TOOL_ARCHIVE $TOOL_URL --post-data "project=$COVERITY_SCAN_PROJECT_NAME&token=$COVERITY_SCAN_TOKEN"
                            fi

                            # Extract Coverity Scan Analysis Tool
                            echo "\033[33;1mExtracting Coverity Scan Analysis Tool...\033[0m"
                            mkdir -p "${COVERITY_TOOL_BASE}" && (cd "${COVERITY_TOOL_BASE}" && tar zxf "${TOOL_ARCHIVE}")
                        fi

                        export PATH="$(find $COVERITY_TOOL_BASE -type d -name 'cov-analysis*')/bin:$PATH"

                        cov-configure --comptype gcc --compiler $(which "${CC}")
                        ;;
                esac
                ;;
            "osx")
                brew update
                brew install \
                     ragel \
                     glib

                case "${COMPILER}" in
                    "gcc-4.6")
                        which gcc-4.6 || brew install homebrew/versions/gcc46
                        ;;
                    "gcc-4.8")
                        which gcc-4.8 || brew install homebrew/versions/gcc48
                        ;;
                    "gcc-5")
                        which gcc-5 || brew install homebrew/versions/gcc5
                        ;;
                esac
                ;;
            *)
                echo "Unknown OS!"
                exit 1
                ;;
        esac
        ;;

    "configure")
        COMMON_COMPILER_FLAGS="-Werror -fno-omit-frame-pointer -fstack-protector-all -D_FORTIFY_SOURCE=2"
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

        case "${COMPILER}" in
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
                ;;
        esac

        git submodule update --init --recursive
        echo "cmake . ${CONFIGURE_FLAGS} -DCMAKE_C_FLAGS=\"${COMMON_COMPILER_FLAGS}\" -DCMAKE_CXX_FLAGS=\"${COMMON_COMPILER_FLAGS}\""
        cmake . ${CONFIGURE_FLAGS} -DCMAKE_C_FLAGS="${COMMON_COMPILER_FLAGS}" -DCMAKE_CXX_FLAGS="${COMMON_COMPILER_FLAGS}"
        ;;

    "make")
        case "${BUILD_TYPE}" in
            "coverity")
                export PATH="$(find $COVERITY_TOOL_BASE -type d -name 'cov-analysis*')/bin:$PATH"

                cov-build --dir cov-int make VERBOSE=1
                # cov-import-scm --dir cov-int --scm git --log cov-int/scm_log.txt || true

                tar czf squash.tgz cov-int

                curl \
                    --silent --write-out "\n%{http_code}\n" \
                    --form project="${COVERITY_SCAN_PROJECT_NAME}" \
                    --form token="${COVERITY_SCAN_TOKEN}" \
                    --form email="evan@coeus-group.com" \
                    --form file="@squash.tgz" \
                    --form version="${TRAVIS_BUILD_NUMBER}/$(git rev-parse --short HEAD)" \
                    --form description="Travis CI build" \
                    "https://scan.coverity.com/builds"
                ;;
            *)
                make VERBOSE=1
                ;;
        esac
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
