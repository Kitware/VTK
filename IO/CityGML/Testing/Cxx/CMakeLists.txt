ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/CityGML/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/CityGML/textures/,REGEX:.*}"
)

vtk_add_test_cxx(vtkIOCityGMLCxxTests tests
  TestCityGMLReader.cxx
  )
vtk_test_cxx_executable(vtkIOCityGMLCxxTests tests)
