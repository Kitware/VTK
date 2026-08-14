vtk_add_test_cxx(vtkViewsScivisCxxTests tests
  TestAnnotationRepresentation.cxx,NO_VALID
  TestSetterModifiedTime.cxx,NO_VALID
  TestScivisView.cxx
  TestScivisExporter.cxx,NO_VALID
  TestScivisViewCamera.cxx,NO_VALID
  TestScivisViewSelection.cxx,NO_VALID
  TestSurfaceRepresentation.cxx
  TestScivisScalarBars.cxx,NO_VALID
  TestTextOverlayRepresentation.cxx,NO_VALID
  TestViewLighting.cxx,NO_VALID
  TestVolumeRepresentation.cxx
  )

vtk_test_cxx_executable(vtkViewsScivisCxxTests tests
  RENDERING_FACTORY)
