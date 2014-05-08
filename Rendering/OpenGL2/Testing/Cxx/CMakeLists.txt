vtk_add_test_cxx(
  TestRenderWidget.cxx
  TestVBOPLYMapper.cxx
  TestVBOPointsLines.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
