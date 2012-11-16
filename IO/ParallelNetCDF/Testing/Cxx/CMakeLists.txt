create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestPNetCDFPOPReader.cxx

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

if(VTK_LARGE_DATA_ROOT)
  add_test(NAME vtkIOParallelNetCDFCxx-TestPNetCDFPOPReader
    COMMAND ${vtk-module}CxxTests TestPNetCDFPOPReader
    -D ${VTK_LARGE_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/IO/TestNetCDFPOPReader.png)
endif(VTK_LARGE_DATA_ROOT)