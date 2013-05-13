vtk_add_test_cxx(Graphics
  TestLineSource.cxx,NO_VALID
  TestTessellatedBoxSource.cxx,NO_VALID
  TestMultiBlock.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
