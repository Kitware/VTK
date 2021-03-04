include(vtkExternalData)

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
