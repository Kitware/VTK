
find_library(LZMA_LIBRARIES NAMES lzma)
find_path(LZMA_INCLUDE_DIRS NAMES lzma.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZMA
  DEFAULT_MSG
  LZMA_LIBRARIES
  LZMA_INCLUDE_DIRS)
