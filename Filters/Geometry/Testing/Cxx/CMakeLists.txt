vtk_add_test_cxx(
  TestExtractSurfaceNonLinearSubdivision.cxx
  TestImageDataToUniformGrid.cxx,NO_VALID
  TestProjectSphereFilter.cxx,NO_VALID
  TestStructuredAMRNeighbor.cxx,NO_VALID
  TestUniformGridGhostDataGenerator.cxx,NO_VALID
  TestUnstructuredGridGeometryFilter.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)

## list of tests that are not included in the main driver
set(MyTests
  TestStructuredAMRGridConnectivity.cxx
  TestStructuredGridConnectivity.cxx
  TestStructuredGridGhostDataGenerator.cxx
  )

foreach(test ${MyTests})
  get_filename_component(TName ${test} NAME_WE)
  add_executable(${TName} ${test})
  target_link_libraries(${TName} vtkFiltersGeometry vtkIOXML)
  add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${TName})
endforeach()
