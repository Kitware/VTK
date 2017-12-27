vtk_add_test_cxx(vtkCommonMiscCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestPolygonBuilder.cxx
  TestPolygonBuilder2.cxx
  TestPolygonBuilder3.cxx
  TestPolygonBuilder4.cxx
  TestResourceFileLocator.cxx
  TestPolygonBuilder5.cxx
  UnitTestFunctionParser.cxx
  )
vtk_test_cxx_executable(vtkCommonMiscCxxTests tests)
