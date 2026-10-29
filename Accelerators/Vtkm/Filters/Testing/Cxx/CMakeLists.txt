
vtk_add_test_cxx(vtkAcceleratorsVTKmFiltersCxxTests tests
  TestVTKMAbort.cxx,NO_VALID
  TestVTKMCleanGrid.cxx
  TestVTKMCoordinateSystemTransform.cxx,NO_VALID
  TestVTKMClip.cxx
  TestVTKMClipWithImplicitFunction.cxx,NO_SERDES
  TestVTKMDeviceResidentOutput.cxx,NO_VALID
  TestVTKMExternalFaces.cxx,NO_SERDES
  TestVTKMExtractVOI.cxx,NO_SERDES
  TestVTKMGradient.cxx,NO_VALID
  TestVTKMGradientAndVorticity.cxx,NO_VALID
  TestVTKMHistogram.cxx,NO_VALID
  TestVTKMHistogramSampling.cxx,NO_VALID
  TestVTKMLevelOfDetail.cxx,NO_SERDES
  TestVTKMMarchingCubes.cxx,NO_SERDES
  TestVTKMMarchingCubes2.cxx,NO_SERDES
  TestVTKMNDHistogram.cxx,NO_VALID
  TestVTKMOverride.cxx,NO_VALID
  TestVTKMPointElevation.cxx,NO_SERDES
  TestVTKMPointTransform.cxx,NO_SERDES
  TestVTKMProbe.cxx,NO_VALID
  TestVTKMPolyDataNormals.cxx
  TestVTKMSlice.cxx,NO_VALID
  TestVTKMThreshold.cxx,NO_SERDES
  TestVTKMThreshold2.cxx,NO_SERDES
  TestVTKMTriangleMeshPointNormals.cxx,NO_SERDES
  TestVTKMWarpScalar.cxx,NO_SERDES
  TestVTKMWarpVector.cxx,NO_SERDES
  )


vtk_test_cxx_executable(vtkAcceleratorsVTKmFiltersCxxTests tests
  RENDERING_FACTORY
  )
