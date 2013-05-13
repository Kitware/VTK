set(tests
  TestAMRGhostLayerStripping
  TestAMRBlanking
  TestAMRIterator
  TestImageToAMR
  )
foreach(t ${tests})
  vtk_add_test_cxx(${t}.cxx NO_VALID NO_OUTPUT)
  set_property(TEST ${vtk-module}Cxx-${t} PROPERTY FAIL_REGULAR_EXPRESSION "Error")
endforeach()
vtk_test_cxx_executable(${vtk-module}CxxTests)
