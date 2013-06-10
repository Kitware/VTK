vtk_add_test_cxx(
  TestLineSource.cxx,NO_VALID
  TestTessellatedBoxSource.cxx,NO_VALID
  TestMultiBlock.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
