#!/bin/sh

git submodule update --init --recursive
./gitlog-to-changelog > ChangeLog
autoreconf -iv || exit 1
./configure --enable-maintainer-mode "$@"
