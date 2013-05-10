vtk_add_test_cxx(Graphics
  TestLineSource.cxx
  TestTessellatedBoxSource.cxx
  TestMultiBlock.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
