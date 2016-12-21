
find_library(LZ4_LIBRARIES NAMES lz4)
find_path(LZ4_INCLUDE_DIRS NAMES lz4.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4
  DEFAULT_MSG
  LZ4_LIBRARIES
  LZ4_INCLUDE_DIRS)
