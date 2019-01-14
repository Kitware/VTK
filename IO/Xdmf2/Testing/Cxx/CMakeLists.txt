vtk_module_test_data(
  Data/XDMF/,REGEX:.*)

vtk_add_test_cxx(vtkIOXdmf2CxxTests tests
  TestTemporalXdmfReaderWriter.cxx,NO_VALID
  XdmfTestVTKIO.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOXdmf2CxxTests tests)
