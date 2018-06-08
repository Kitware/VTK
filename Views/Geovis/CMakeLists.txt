if(NOT VTK_LEGACY_REMOVE)
  set(Module_SRCS
    vtkGeoView.cxx
    vtkGeoView2D.cxx
    )

  vtk_module_library(vtkViewsGeovis ${Module_SRCS})
endif()
