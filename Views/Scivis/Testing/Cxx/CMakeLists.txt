vtk_add_test_cxx(vtkViewsScivisCxxTests tests
  TestAnnotationRepresentation.cxx,NO_VALID
  TestGridAxesRepresentation.cxx,NO_VALID
  TestSetterModifiedTime.cxx,NO_VALID
  TestScivisView.cxx
  TestScivisExporter.cxx,NO_VALID
  TestScivisViewCamera.cxx,NO_VALID
  TestScivisViewSelection.cxx,NO_VALID
  TestSurfaceRepresentation.cxx
  TestSurfaceRepresentationColoring.cxx,NO_VALID
  TestScivisScalarBars.cxx,NO_VALID
  TestTextOverlayRepresentation.cxx,NO_VALID
  TestViewLighting.cxx,NO_VALID
  TestVolumeRepresentation.cxx
  )

if (VTK_WRAP_SERIALIZATION)
  vtk_add_test_cxx(vtkViewsScivisCxxTests tests
    TestScivisViewSerialization.cxx,NO_VALID)
endif ()

vtk_test_cxx_executable(vtkViewsScivisCxxTests tests
  RENDERING_FACTORY)
