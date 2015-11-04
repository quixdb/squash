#!/bin/sh

# This script only exists to support Travis CI, since (AFAICT) it is
# not possible to perform different before_install steps depending on
# the operating system.  You should not be using this script.

export GCOV=gcov

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
                exit -1
                ;;
        esac
        ;;

    "configure")
        COMMON_COMPILER_FLAGS="-Werror -fno-omit-frame-pointer -fstack-protector-all"
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
            "release")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} --disable-debug"
                ;;
            "coverage")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} --enable-coverage"
                ;;
        esac

        case "${TRAVIS_OS_NAME}" in
            "osx")
                CONFIGURE_FLAGS="${CONFIGURE_FLAGS} --disable-ms-compress --disable-ncompress --disable-lzham"
                ;;
        esac

        git submodule update --init --recursive
        /bin/bash -x ./autogen.sh \
                  ${CONFIGURE_FLAGS} \
                  CFLAGS="${COMMON_COMPILER_FLAGS}" \
                  CXXFLAGS="${COMMON_COMPILER_FLAGS}"
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

        case "${BUILD_TYPE}" in
            "coverage")
                coveralls --gcov "${GCOV}" -e CMakeFiles -e plugins -e examples -e tests -e squash/tinycthread
                ;;
        esac
        ;;
    *)
        echo "Unknown step."
        exit -1
        ;;
esac
