if(VTK_DATA_ROOT)
  vtk_add_test_cxx(IO
    TestAMRXMLIO.cxx
    TestXMLHierarchicalBoxDataFileConverter.cxx
    TestXMLUnstructuredGridReader.cxx
    )
  set(TestXML_ARGS ${VTK_DATA_ROOT}/Data/sample.xml)
  vtk_add_test_cxx(TestXML.cxx NO_DATA NO_VALID NO_OUTPUT)
endif()

vtk_add_test_cxx(IO
  TestDataObjectXMLIO.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
