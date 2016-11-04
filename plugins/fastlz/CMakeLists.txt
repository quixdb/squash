include (SquashPlugin)

find_path(FASTLZ_INCLUDE_DIR fastlz.h PATH_SUFFIXES include)
find_library (FASTLZ_LIBRARIES NAMES fastlz)

find_package_handle_standard_args(FASTLZ
  REQUIRED_VARS
    FASTLZ_INCLUDE_DIR
    FASTLZ_LIBRARIES)

squash_plugin (
  NAME fastlz
  EXTERNAL_PKG_PREFIX FASTLZ
  SOURCES squash-fastlz.c
  EMBED_SOURCES fastlz/fastlz.c
  EMBED_INCLUDE_DIRS fastlz)
