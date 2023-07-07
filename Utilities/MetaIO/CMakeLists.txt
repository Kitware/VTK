set(METAIO_FOR_VTK 1)
set(VTK_ZLIB_LIBRARIES vtkzlib)
if(VTK_REQUIRE_LARGE_FILE_SUPPORT)
  add_definitions(
    -D_LARGEFILE_SOURCE
    -D_LARGEFILE64_SOURCE
    -D_LARGE_FILES
    -D_FILE_OFFSET_BITS=64)
endif()
# We never want MetaIO's tests.
set(BUILD_TESTING OFF)

# FIXME: MetaIO only exports symbols properly on Windows.
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)

vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkmetaio/License.txt"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause AND BSD-4-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright 2000-2014 Insight Software Consortium"
    "Copyright (c) 1999-2007 Insight Software Consortium"
  SPDX_DOWNLOAD_LOCATION
    "git+https://github.com/Kitware/MetaIO.git@ea2bb780"
  VERSION
    "master"
  STANDARD_INCLUDE_DIRS)
