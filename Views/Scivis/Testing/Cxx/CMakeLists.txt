vtk_add_test_cxx(vtkViewsScivisCxxTests tests
  TestSetterModifiedTime.cxx,NO_VALID
  TestStandardRenderView.cxx
  TestStandardRenderViewSelection.cxx,NO_VALID
  TestSurfaceRepresentation.cxx
  TestVolumeRepresentation.cxx
  )

vtk_test_cxx_executable(vtkViewsScivisCxxTests tests
  RENDERING_FACTORY)
