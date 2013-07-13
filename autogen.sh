./gitlog-to-changelog > ChangeLog
autoreconf -iv || exit 1
./configure --enable-maintainer-mode "$@"
