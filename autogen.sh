#!/bin/sh

git submodule update --init --recursive
cmake "$@" .
