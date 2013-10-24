#!/bin/sh

git submodule update --init --recursive

if [ ! -e build ]; then
    mkdir build
fi
(cd build && cmake ..)
