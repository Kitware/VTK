vtk_add_test_cxx(vtkFiltersGenericCxxTests tests
  TestGenericStreamTracer.cxx
  TestGenericClip.cxx
  TestGenericContourFilter.cxx
  TestGenericCutter.cxx
  TestGenericDataSetTessellator.cxx
  TestGenericGeometryFilter.cxx
  TestGenericGlyph3DFilter.cxx
  TestGenericProbeFilter.cxx
  otherCreation.cxx,NO_VALID
  UnitTestGenericGeometryFilter.cxx,NO_VALID,NO_DATA
  )

vtk_test_cxx_executable(vtkFiltersGenericCxxTests tests)
