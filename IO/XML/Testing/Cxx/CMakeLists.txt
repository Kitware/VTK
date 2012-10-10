create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestAMRXMLIO.cxx
  TestDataObjectXMLIO.cxx
  TestXML.cxx
  TestXMLHierarchicalBoxDataFileConverter.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)
list(REMOVE_ITEM TestsToRun TestXML.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/IO/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-XML
    COMMAND ${vtk-module}CxxTests TestXML ${VTK_DATA_ROOT}/Data/sample.xml)
endif()
