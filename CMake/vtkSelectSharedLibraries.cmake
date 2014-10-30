IF(NOT VTK_SHARED_LIBRARIES_SELECTED)
  SET(VTK_SHARED_LIBRARIES_SELECTED 1)

  # We need the VTK_DEPENDENT_OPTION macro.
  INCLUDE(${VTK_CMAKE_DIR}/vtkDependentOption.cmake)

  # Choose static or shared libraries.
  OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
  SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

ENDIF()
