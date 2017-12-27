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

vtk_add_test_cxx(vtkFiltersAMRCxxTests tests
  NO_VALID NO_OUTPUT
  ${test_files}
  )
vtk_test_cxx_executable(vtkFiltersAMRCxxTests tests)

foreach(test IN LISTS cxx_tests)
  set_tests_properties(vtkFiltersAMRCxx-${test}
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "Error"
  )
endforeach()
