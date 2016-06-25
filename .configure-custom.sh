#!/bin/sh

DISABLE_VARS="external|yes|FORCE_IN_TREE_DEPENDENCIES"
DISABLE_FORCE_IN_TREE_DEPENDENCIES_DOC="force in-tree dependencies, even when a system library is available"

for plugin in \
    brieflz \
    brotli \
    bsc \
    bzip2 \
    copy \
    crush \
    fari \
    fastlz \
    gipfeli \
    heatshrink \
    libdeflate \
    lz4 \
    lzf \
    lzfse \
    lzg \
    lzham \
    lzjb \
    lzma \
    lzo \
    miniz \
    ms-compress \
    ncompress \
    quicklz \
    snappy \
    wflz \
    wimlib \
    yalz77 \
    zlib \
    zlib-ng \
    zling \
    zpaq \
    zstd; do
  NAME_UC=$(echo $plugin | tr '[:lower:]' '[:upper:]' | sed 's/[^A-Z0-9]/_/g')
  DISABLE_VARS="${DISABLE_VARS} ${plugin}|no|ENABLE_${NAME_UC}"
  eval "DISABLE_ENABLE_${NAME_UC}_DOC=\"disable the ${plugin} plugin\""
done

ENABLE_VARS="coverage|yes|ENABLE_COVERAGE undefined|yes|ALLOW_UNDEFINED installed-tests|yes|ENABLE_INSTALLED_TESTS"
ENABLE_ENABLE_COVERAGE_DOC="enable coverage analysis"
ENABLE_ALLOW_UNDEFINED_DOC="allow plugins to leverage undefined behavior"
ENABLE_ENABLE_INSTALLED_TESTS_DOC="install unit tests"

for plugin in \
    csc \
    density \
    doboz; do
  NAME_UC=$(echo $plugin | tr '[:lower:]' '[:upper:]' | sed 's/[^A-Z0-9]/_/g')
  ENABLE_VARS="${ENABLE_VARS} ${plugin}|yes|ENABLE_${NAME_UC}"
  eval "ENABLE_ENABLE_${NAME_UC}_DOC=\"enable the ${plugin} plugin (disabled due to bugs)\""
done

WITH_VARS="plugin-dir|path|PLUGIN_DIRECTORY search-path|path|SEARCH_PATH"
WITH_PLUGIN_DIRECTORY_DOC="directory to install plugins to [LIBDIR/squash/API_VERSION/plugins]"
WITH_SEARCH_PATH_DOC="directory to search for plugins by default"
