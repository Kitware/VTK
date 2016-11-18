# Find the GLEW library
#
# Defines:
#
#  GLEW_FOUND        - system has GLEW
#  GLEW_INCLUDE_DIRS - the GLEW include directories
#  GLEW_LIBRARY      - The GLEW library
#
find_path(GLEW_INCLUDE_DIR GL/glew.h)
find_library(GLEW_LIBRARY NAMES GLEW glew32)

set(GLEW_INCLUDE_DIRS "${GLEW_INCLUDE_DIR}")
set(GLEW_LIBRARIES "${GLEW_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_INCLUDE_DIR GLEW_LIBRARY)

mark_as_advanced(GLEW_INCLUDE_DIR GLEW_LIBRARY)
