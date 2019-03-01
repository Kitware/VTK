include(vtkExternalData)

# Test input data staging directory.
file(RELATIVE_PATH vtk_reldir "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
set(VTK_TEST_DATA_DIR "${ExternalData_BINARY_ROOT}/${vtk_reldir}/Testing")

# Test input data directory.
set(VTK_TEST_INPUT_DIR "${VTK_SOURCE_DIR}/Testing/Data")

# Test output directory.
set(VTK_TEST_OUTPUT_DIR "${VTK_BINARY_DIR}/Testing/Temporary")

if(NOT EXISTS "${VTK_SOURCE_DIR}/.ExternalData/README.rst")
  # This file is always present in version-controlled source trees
  # so we must have been extracted from a source tarball with no
  # data objects needed for testing.  Turn off tests by default
  # since enabling them requires network access or manual data
  # store configuration.
  set(VTK_BUILD_TESTING OFF)
endif()
include(CTest)
set_property(CACHE BUILD_TESTING
  PROPERTY
    TYPE INTERNAL)

# Provide an option for tests requiring "large" input data
option(VTK_USE_LARGE_DATA "Enable tests requiring \"large\" data" OFF)
