#!/bin/sh

for submodule in `grep -oE 'path = (.+)' .gitmodules | sed -E -e 's/^path = (.+)$/\1/'`; do
  echo "Checking $submodule"
  if [ ! -d "${submodule}/.git" ]; then
    git submodule init
    git submodule update
  fi
done

./gitlog-to-changelog > ChangeLog
autoreconf -iv || exit 1
./configure --enable-maintainer-mode "$@"
