#!/bin/bash

if [ -e $(dirname $0)/.gitmodules ]; then
    git submodule update --init --recursive
fi

$(dirname $0)/configure "$@"
