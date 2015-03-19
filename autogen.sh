#!/bin/bash

if [ -e $(dirname $0)/.gitmodules ]; then
    (cd "$( dirname "${BASH_SOURCE[0]}" )" && git submodule update --init --recursive)
fi

$(dirname $0)/configure "$@"
