# Tests with data
vtk_add_test_cxx(TestGenericStreamTracer.cxx)
vtk_add_test_cxx(
  TestGenericClip.cxx
  TestGenericContourFilter.cxx
  TestGenericCutter.cxx
  TestGenericDataSetTessellator.cxx
  TestGenericGeometryFilter.cxx
  TestGenericGlyph3DFilter.cxx
  TestGenericProbeFilter.cxx
  otherCreation.cxx,NO_VALID
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
