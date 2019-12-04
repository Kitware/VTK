vtk_add_test_cxx(vtkIOInfovisCxxTests tests
  NO_VALID
  TestBiomTableReader.cxx
  TestDIMACSGraphReader.cxx
  TestDataObjectIO.cxx
  TestISIReader.cxx
  TestFixedWidthTextReader.cxx
  TestNewickTreeReader.cxx
  TestNewickTreeWriter.cxx
  TestMultiNewickTreeReader.cxx
  TestPhyloXMLTreeReadWrite.cxx
  TestRISReader.cxx
  TestTulipReaderProperties.cxx
  TestDelimitedTextReader2.cxx
  TestTemporalDelimitedTextReader.cxx
  )
vtk_test_cxx_executable(vtkIOInfovisCxxTests tests)
