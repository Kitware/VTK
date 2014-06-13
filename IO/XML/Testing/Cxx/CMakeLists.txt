set(TestXML_ARGS "DATA{${VTK_TEST_INPUT_DIR}/sample.xml}")
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestAMRXMLIO.cxx,NO_VALID
  TestHyperOctreeIO.cxx
  TestXMLHierarchicalBoxDataFileConverter.cxx,NO_VALID
  TestXMLUnstructuredGridReader.cxx
  TestXML.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestDataObjectXMLIO.cxx,NO_VALID
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
