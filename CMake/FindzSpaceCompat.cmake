#[=======================================================================[
FindzSpace
--------

Find the zSpace Core Compatibility SDK. This is the newest version of the zSpace SDK,
intended to work with all zSpace hardware, including the newest (like the Inspire).

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines 3 targets if zSpace has been found:

``zSpaceHeaders``
  zSpace header files
``zSpaceImpl``
  zSpace library (dll). 
  We don't use any shared library at compile time.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``zSpaceCompat_FOUND``
  True if zSpace is found.
``zSpace_INCLUDE_DIRS``
  Path to include directory for zSpace headers.
``zSpace_LIBRARIES``
  Path to zSpace library (.dll).

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``zSpace_INCLUDE_DIR``
  Path to include directory for zSpace headers.
``zSpace_LIBRARY``
  Path to zSpace library (.dll).

Hints
^^^^^

Set ``zSpace_DIR`` or ``zSpace_ROOT`` in the environment to specify the
zSpace installation prefix.
example : set zSpace_DIR=C:/zSpace/zSpaceSdks/CoreCompatibility_1.0.0.12
#]=======================================================================]

# Find zSpace headers folder
find_path(zSpace_INCLUDE_DIR 
  NAMES 
    zSpaceCoreCompatibility.h
  HINTS
    ${zSpace_ROOT}
    ENV zSpace_DIR
    ENV zSpace_ROOT
  PATH_SUFFIXES
    zSpaceSdks
    include
    Inc
  DOC "Path to the zSpace Core Compatibility SDK include directory"
)
mark_as_advanced(zSpace_INCLUDE_DIR)

# Find zSpace library
find_file(zSpace_LIBRARY
  NAMES
    zSpaceCoreCompatibility64.dll
    zSpaceCoreCompatibility.dll
  HINTS
    ${zSpace_ROOT}
    ENV zSpace_DIR
    ENV zSpace_ROOT
  PATH_SUFFIXES
    zSpaceSdks
    Bin
  DOC "Path to the zSpace Core Compatibility SDK library"
)
mark_as_advanced(zSpace_LIBRARY)

# Handle some find_package arguments like REQUIRED
# and set the zSpace_FOUND variable
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zSpaceCompat
    REQUIRED_VARS zSpace_LIBRARY zSpace_INCLUDE_DIR)

if (zSpaceCompat_FOUND)
  if (NOT TARGET zSpaceHeaders)
    # Interface library (headers only)
    add_library(zSpaceHeaders INTERFACE IMPORTED)
    target_include_directories(zSpaceHeaders 
      INTERFACE "${zSpace_INCLUDE_DIR}")
      
    # For later purpose (install, etc.)
    add_library(zSpaceImpl MODULE IMPORTED)
    set_target_properties(zSpaceImpl PROPERTIES
      IMPORTED_LOCATION "${zSpace_LIBRARY}")
  endif ()
endif ()
