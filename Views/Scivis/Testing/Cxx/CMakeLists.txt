vtk_add_test_cxx(vtkViewsScivisCxxTests tests
  TestAnnotationRepresentation.cxx,NO_VALID
  TestSetterModifiedTime.cxx,NO_VALID
  TestScivisView.cxx
  TestScivisViewSelection.cxx,NO_VALID
  TestSurfaceRepresentation.cxx
  TestViewLighting.cxx,NO_VALID
  TestVolumeRepresentation.cxx
  )

vtk_test_cxx_executable(vtkViewsScivisCxxTests tests
  RENDERING_FACTORY)
