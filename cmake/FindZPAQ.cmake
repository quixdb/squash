find_path(ZPAQ_INCLUDE_DIR libzpaq.h PATH_SUFFIXES include)
find_library (ZPAQ_LIBRARIES NAMES zpaq)

find_package_handle_standard_args(ZPAQ
  REQUIRED_VARS
    ZPAQ_INCLUDE_DIR
    ZPAQ_LIBRARIES)
