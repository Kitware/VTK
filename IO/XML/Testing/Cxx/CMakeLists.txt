set(TestXML_ARGS "DATA{${VTK_TEST_INPUT_DIR}/sample.xml}")
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestAMRXMLIO.cxx,NO_VALID
  TestHyperOctreeIO.cxx
  TestXMLHierarchicalBoxDataFileConverter.cxx,NO_VALID
  TestXMLUnstructuredGridReader.cxx
  TestXML.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestDataObjectXMLIO.cxx,NO_VALID
  )

# Each of these most be added in a separate vtk_add_test_cxx
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestXMLReaderBadImageData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${VTK_TEST_INPUT_DIR}/badImageData.xml}"
)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestXMLReaderBadPolyData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${VTK_TEST_INPUT_DIR}/badPolyData.xml}"
)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestXMLReaderBadRectilinearGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${VTK_TEST_INPUT_DIR}/badRectilinearGridData.xml}"
)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestXMLReaderBadUnstucturedGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${VTK_TEST_INPUT_DIR}/badUnstructuredGridData.xml}"
)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestXMLReaderBadUniformGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${VTK_TEST_INPUT_DIR}/badUniformGridData.xml}"
)

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
