vtk_add_test_cxx(${vtk-module}CxxTests data_tests
  # TestImageReader2Factory.cxx   # fixme (deps not satisfied)
  TestNrrdReader.cxx
  )
set(TestMetaIO_ARGS "DATA{${VTK_TEST_INPUT_DIR}/HeadMRVolume.mhd,HeadMRVolume.raw}")
vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDataObjectIO.cxx
  TestMetaIO.cxx
  TestImportExport.cxx
  )

set(all_tests
  ${data_tests}
  ${tests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests all_tests)
