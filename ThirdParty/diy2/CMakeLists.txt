# Diy2 does not work on windows
if (NOT WIN32)
  vtk_module_third_party(diy2
    INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/vtkdiy2/include"
    NO_LIBRARIES
    )
endif()
