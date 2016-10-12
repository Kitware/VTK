if(ANDROID OR APPLE_IOS) # No GL2PS on mobile
  return()
endif()
vtk_module(vtkgl2ps
  EXCLUDE_FROM_WRAPPING
  DEPENDS
    vtkpng
    vtkzlib
)