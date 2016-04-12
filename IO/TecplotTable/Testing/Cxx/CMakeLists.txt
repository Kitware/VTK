
vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestTecplotTableReader.cxx
  )

ExternalData_add_test(VTKData
  NAME TestTecplotTableReader
  COMMAND ${vtk-module}CxxTests TestTecplotTableReader
  -D DATA{../Data/residuals.dat}
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
