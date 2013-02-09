# Tests with data
if(VTK_DATA_ROOT)
  set(NEEDS_DATA
  TestGenericStreamTracer.cxx
  )
endif()
vtk_tests(BASELINEDIR GenericFiltering DATADIR ${VTK_DATA_ROOT}
  ${NEEDS_DATA}
  TestGenericClip.cxx
  TestGenericContourFilter.cxx
  TestGenericCutter.cxx
  TestGenericDataSetTessellator.cxx
  TestGenericGeometryFilter.cxx
  TestGenericGlyph3DFilter.cxx
  TestGenericProbeFilter.cxx
  otherCreation.cxx)
