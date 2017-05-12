
find_package(VTKm REQUIRED COMPONENTS Base)
include_directories(${VTKm_INCLUDE_DIRS})

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestVTKMCleanGrid.cxx
  TestVTKMClip.cxx
  TestVTKMClipWithImplicitFunction.cxx
  # TestVTKMGradientAndVorticity.cxx,NO_VALID
  TestVTKMExternalFaces.cxx
  TestVTKMLevelOfDetail.cxx
  TestVTKMMarchingCubes.cxx
  TestVTKMMarchingCubes2.cxx
  TestVTKMThreshold.cxx
  TestVTKMThreshold2.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
