vtk_add_test_cxx(
  TestAMRXMLIO.cxx,NO_VALID
  TestHyperOctreeIO.cxx
  TestXMLHierarchicalBoxDataFileConverter.cxx,NO_VALID
  TestXMLUnstructuredGridReader.cxx
  )
set(TestXML_ARGS "DATA{${VTK_TEST_INPUT_DIR}/sample.xml}")
vtk_add_test_cxx(TestXML.cxx NO_DATA NO_VALID NO_OUTPUT)

vtk_add_test_cxx(NO_VALID
  TestDataObjectXMLIO.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
