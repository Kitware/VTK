ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/SegY/,REGEX:.*}"
)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestSegY2DReader.cxx
  TestSegY2DReaderZoom.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
