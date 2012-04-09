vtk_module(vtkFiltersHybrid
  GROUPS
    StandAlone
  DEPENDS
    vtkImagingSources
    vtkRenderingOpenGL # This should not be allowed - FIXME - move classes.
    vtkFiltersGeneral
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString # This is only here as GL is in this module. FIXME.
  TEST_DEPENDS
    vtkImagingCore
    vtkTestingRendering
  )
