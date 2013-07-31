#!/bin/sh

for submodule in plugins/lz4/lz4; do
  if [ ! -d "${submodule}/.git" ]; then
    git submodule init
    git submodule update
  fi
done

./gitlog-to-changelog > ChangeLog
autoreconf -iv || exit 1
./configure --enable-maintainer-mode "$@"
