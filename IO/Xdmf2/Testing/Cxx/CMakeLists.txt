ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/XDMF/,REGEX:.*}"
  )

vtk_add_test_cxx(vtkIOXdmf2CxxTests tests
  TestTemporalXdmfReaderWriter.cxx,NO_VALID
  XdmfTestVTKIO.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOXdmf2CxxTests tests)
