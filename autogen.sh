#!/bin/sh

git submodule update --init --recursive
./gitlog-to-changelog > ChangeLog

if [ ! -e build ]; then
    mkdir build
fi
cd build && cmake ..
