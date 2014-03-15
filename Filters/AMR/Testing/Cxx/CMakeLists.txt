set(cxx_tests
  TestAMRGhostLayerStripping
  TestAMRBlanking
  TestAMRIterator
  TestImageToAMR
  )
set(test_files)
foreach(test IN LISTS cxx_tests)
  list(APPEND test_files
    ${test}.cxx)
endforeach()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT
  ${test_files}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)

foreach(test IN LISTS cxx_tests)
  set_tests_properties(${vtk-module}Cxx-${test}
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "Error"
  )
endforeach()
