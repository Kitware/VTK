vtk_add_test_cxx(vtkViewsRenderingCxxTests tests
  TestSetterModifiedTime.cxx,NO_VALID
  TestStandardRenderView.cxx
  TestStandardRenderViewSelection.cxx,NO_VALID
  TestSurfaceRepresentation.cxx
  TestVolumeRepresentation.cxx
  )

vtk_test_cxx_executable(vtkViewsRenderingCxxTests tests
  RENDERING_FACTORY)
