
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

  #the tests aren't scoped as a child directory of vtkAcceleratorsVTKm
  #so we need to redo this logic
  get_property(arch_flags
    TARGET    vtkm::cuda
    PROPERTY  INTERFACE_CUDA_Architecture_Flags)
  if(NOT arch_flags)
    message(FATAL_ERROR "VTK-m CUDA Architecture flags can't be found. This means we can't safely compile CUDA code")
  endif()
  string(APPEND CMAKE_CUDA_FLAGS " ${arch_flags}")
endif()


vtk_test_cxx_executable(vtkAcceleratorsVTKmCxxTests tests
  RENDERING_FACTORY
  )

if (TARGET vtkm::cuda)
  # When cuda is enabled VTK::AcceleratorsVTKm is built statically but with fpic
  # enabled so the tests are also built with fpic enabled
  set_target_properties(vtkAcceleratorsVTKmCxxTests PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
