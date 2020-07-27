
vtk_add_test_cxx(vtkAcceleratorsVTKmFiltersCxxTests tests
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

  #the tests aren't scoped as a child directory of vtkAcceleratorsVTKmFilters
  #so we need to redo this logic
  vtkm_get_cuda_flags(CMAKE_CUDA_FLAGS)

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif()


vtk_test_cxx_executable(vtkAcceleratorsVTKmFiltersCxxTests tests
  RENDERING_FACTORY
  )

if (TARGET vtkm::cuda)
  # When cuda is enabled VTK::AcceleratorsVTKmFilters is built statically but with fpic
  # enabled so the tests are also built with fpic enabled
  set_target_properties(vtkAcceleratorsVTKmFiltersCxxTests PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
