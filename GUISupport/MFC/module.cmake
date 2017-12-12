# only add this module if we are on a windows platform
if(WIN32)
  vtk_module(vtkGUISupportMFC
    GROUPS
    EXCLUDE_FROM_WRAPPING
    DEPENDS
      vtkRenderingOpenGL2
    )
endif()
