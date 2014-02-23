vtk_add_test_cxx(NO_VALID
  TestBiomTableReader.cxx
  TestDIMACSGraphReader.cxx
  TestDataObjectIO.cxx
  TestISIReader.cxx
  TestFixedWidthTextReader.cxx
  TestNewickTreeReader.cxx
  TestNewickTreeWriter.cxx
  TestMultiNewickTreeReader.cxx
  TestRISReader.cxx
  TestTulipReaderProperties.cxx
  TestDelimitedTextReader2.cxx
  )

# The PhyloXML test is handled separately because its baseline is a .xml file
# (not a png)
set_property(DIRECTORY APPEND PROPERTY VTK_TEST_CXX_SOURCES
  TestPhyloXMLTreeWriter.cxx
  )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-PhyloXMLTreeWriter
  COMMAND ${vtk-module}CxxTests TestPhyloXMLTreeWriter 1
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V DATA{../Data/Baseline/TestPhyloXMLTreeWriter.xml,:})

vtk_test_cxx_executable(${vtk-module}CxxTests)
