
vtk_add_test_cxx(vtkAcceleratorsVTKmFiltersCxxTests tests
  TestVTKMAbort.cxx,NO_VALID
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
  TestVTKMOverride.cxx,NO_VALID
  TestVTKMPointElevation.cxx
  TestVTKMPointTransform.cxx
  TestVTKMProbe.cxx,NO_VALID
  TestVTKMPolyDataNormals.cxx
  TestVTKMSlice.cxx,NO_VALID
  TestVTKMThreshold.cxx
  TestVTKMThreshold2.cxx
  TestVTKMTriangleMeshPointNormals.cxx
  TestVTKMWarpScalar.cxx
  TestVTKMWarpVector.cxx
  )

if (TARGET viskores::cuda)
  foreach(src IN LISTS tests)
    string(REPLACE "," ";" src ${src})
    list(GET src 0 src)

    set_source_files_properties(${src} PROPERTIES LANGUAGE CUDA)
  endforeach()

  #the tests aren't scoped as a child directory of vtkAcceleratorsVTKmFilters
  #so we need to redo this logic
  viskores_get_cuda_flags(CMAKE_CUDA_FLAGS)

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")

endif()


vtk_test_cxx_executable(vtkAcceleratorsVTKmFiltersCxxTests tests
  RENDERING_FACTORY
  )

if (TARGET viskores::cuda)
  set_target_properties(vtkAcceleratorsVTKmFiltersCxxTests PROPERTIES
    CUDA_ARCHITECTURES OFF)
endif()
