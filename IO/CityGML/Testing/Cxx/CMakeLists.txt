vtk_add_test_cxx(vtkIOCityGMLCxxTests tests
  TestCityGMLReader.cxx
  TestCityGMLReaderStream.cxx
  )
vtk_test_cxx_executable(vtkIOCityGMLCxxTests tests)
