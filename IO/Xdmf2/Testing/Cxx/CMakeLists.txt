vtk_module_test_executable(XdmfTestVTKIO XdmfTestVTKIO.cxx)

add_test(NAME XdmfTestVTKIO
  COMMAND XdmfTestVTKIO
  -T {VTK_TEST_OUTPUT_DIR}
)
