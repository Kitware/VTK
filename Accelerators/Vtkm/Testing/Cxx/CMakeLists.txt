include_directories(${VTKm_INCLUDE_DIRS})

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestVTKMCleanGrid.cxx
  TestVTKMClip.cxx
  TestVTKMClipWithImplicitFunction.cxx
  TestVTKMGradient.cxx,NO_VALID
  TestVTKMGradientAndVorticity.cxx,NO_VALID
  TestVTKMExternalFaces.cxx
  TestVTKMExtractVOI.cxx
  TestVTKMLevelOfDetail.cxx
  TestVTKMMarchingCubes.cxx
  TestVTKMMarchingCubes2.cxx
  TestVTKMPolyDataNormals.cxx
  TestVTKMThreshold.cxx
  TestVTKMThreshold2.cxx
  TestVTKMTriangleMeshPointNormals.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )

# Allow vtkm_cont library to be used from tests
target_link_libraries(vtkAcceleratorsVTKmCxxTests LINK_PRIVATE ${VTKm_LIBRARIES})
