find_path(GLEW_INCLUDE_DIR
  NAMES GL/glew.h
  DOC "glew include directory")
mark_as_advanced(GLEW_INCLUDE_DIR)
find_library(GLEW_LIBRARY
  NAMES GLEW glew32
  DOC "glew library")
mark_as_advanced(GLEW_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW REQUIRED_VARS GLEW_LIBRARY GLEW_INCLUDE_DIR)

if (GLEW_FOUND)
  set(GLEW_INCLUDE_DIRS "${GLEW_INCLUDE_DIR}")
  set(GLEW_LIBRARIES "${GLEW_LIBRARY}")

  if (NOT TARGET GLEW::GLEW)
    include(vtkDetectLibraryType)
    vtk_detect_library_type(glew_library_type
      PATH "${GLEW_LIBRARY}")
    add_library(GLEW::GLEW "${glew_library_type}" IMPORTED)
    unset(glew_library_type)
    set_target_properties(GLEW::GLEW PROPERTIES
      IMPORTED_LOCATION "${GLEW_LIBRARY}"
      IMPORTED_IMPLIB "${GLEW_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIR}")
  endif ()
endif ()
