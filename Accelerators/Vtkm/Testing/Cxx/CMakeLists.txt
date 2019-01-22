
vtk_add_test_cxx(vtkAcceleratorsVTKmCxxTests tests
  TestVTKMCleanGrid.cxx
  TestVTKMCoordinateSystemTransform.cxx,NO_VALID
  TestVTKMClip.cxx
  TestVTKMClipWithImplicitFunction.cxx
  TestVTKMExternalFaces.cxx
  TestVTKMExtractVOI.cxx
  TestVTKMGradient.cxx,NO_VALID
  TestVTKMGradientAndVorticity.cxx,NO_VALID
  TestVTKMHistogram.cxx,NO_VALID
  TestVTKMLevelOfDetail.cxx
  TestVTKMMarchingCubes.cxx
  TestVTKMMarchingCubes2.cxx
  TestVTKMNDHistogram.cxx,NO_VALID
  TestVTKMPointElevation.cxx
  TestVTKMPointTransform.cxx
  TestVTKMProbe.cxx,NO_VALID
  TestVTKMPolyDataNormals.cxx
  TestVTKMThreshold.cxx
  TestVTKMThreshold2.cxx
  TestVTKMTriangleMeshPointNormals.cxx
  TestVTKMWarpScalar.cxx
  TestVTKMWarpVector.cxx
  )

if (TARGET vtkm::cuda)
  #the enable_language call is scoped! so we have to re-enable
  #cuda in the test directory
  enable_language(CUDA)
  foreach(src IN LISTS tests)
    set_source_files_properties(${src}.cxx PROPERTIES LANGUAGE "CUDA")
  endforeach()
endif()


vtk_test_cxx_executable(vtkAcceleratorsVTKmCxxTests tests
  RENDERING_FACTORY
  )
