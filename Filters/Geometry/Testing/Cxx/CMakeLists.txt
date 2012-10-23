create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestExtractSurfaceNonLinearSubdivision.cxx
  TestImageDataToUniformGrid.cxx
  TestProjectSphereFilter.cxx
  TestStructuredAMRNeighbor.cxx
  TestUniformGridGhostDataGenerator.cxx
  TestUnstructuredGridGeometryFilter.cxx

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Graphics/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

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
