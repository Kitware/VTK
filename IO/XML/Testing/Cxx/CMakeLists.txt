set(TestXML_ARGS "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/sample.xml}")
set(all_tests
  TestAMRXMLIO.cxx,NO_VALID
  TestDataObjectXMLIO.cxx,NO_VALID
  TestMultiBlockDataSetWithWriteWithEmptyPolyData.cxx,NO_DATA,NO_VALID
  TestMultiBlockXMLIOWithPartialArrays.cxx,NO_VALID
  TestMultiBlockXMLIOWithPartialArraysTable.cxx,NO_VALID
  TestReadDuplicateDataArrayNames.cxx,NO_DATA,NO_VALID
  TestSettingTimeArrayInReader.cxx,NO_VALID,NO_OUTPUT
  TestXML.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestXMLGhostCellsImport.cxx
  TestXMLHierarchicalBoxDataFileConverter.cxx,NO_VALID
  TestXMLHyperTreeGridIO.cxx,NO_VALID
  TestXMLHyperTreeGridIO2.cxx,NO_VALID
  TestXMLHyperTreeGridIOInterface.cxx
  TestXMLHyperTreeGridIOReduction.cxx,NO_VALID
  TestXMLHyperTreeGridReaderV2Bounds.cxx,NO_VALID
  TestXMLLargeUnstructuredGrid.cxx,NO_VALID
  TestXMLMappedUnstructuredGridIO.cxx,NO_DATA,NO_VALID
  TestXMLMultiBlockDataWriterWithEmptyLeaf.cxx,NO_DATA,NO_VALID
  TestXMLPieceDistribution.cxx
  TestXMLPolyhedronUnstructuredGrid.cxx,NO_DATA,NO_VALID
  TestXMLToString.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestXMLUnstructuredGridReader.cxx
  TestXMLWriterWithDataArrayFallback.cxx,NO_VALID
  TestXMLLegacyFileReadIdTypeArrays.cxx,NO_VALID,NO_OUTPUT
  TestXMLWriteTimeValue.cxx,NO_VALID
  )

if ((NOT DEFINED MSVC_VERSION) OR (MSVC_VERSION GREATER 1800))
  # skip TestXMLWriteRead test on MSVC 2013 and older.
  list(APPEND all_tests
    TestXMLWriteRead.cxx,NO_VALID,NO_DATA)
endif()

vtk_add_test_cxx(vtkIOXMLCxxTests tests
  ${all_tests})

# Each of these most be added in a separate vtk_add_test_cxx
vtk_add_test_cxx(vtkIOXMLCxxTests tests
  TestXMLCompositeDataReaderDistribution.cxx,NO_VALID,NO_OUTPUT
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/distTest.vtm,REGEX:distTest_[0-9]_0.vtp}"
  )
vtk_add_test_cxx(vtkIOXMLCxxTests tests
  TestXMLReaderBadImageData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badImageData.xml}"
)
vtk_add_test_cxx(vtkIOXMLCxxTests tests
  TestXMLReaderBadPolyData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badPolyData.xml}"
)
vtk_add_test_cxx(vtkIOXMLCxxTests tests
  TestXMLReaderBadRectilinearGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badRectilinearGridData.xml}"
)
vtk_add_test_cxx(vtkIOXMLCxxTests tests
  TestXMLReaderBadUnstucturedGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badUnstructuredGridData.xml}"
)
vtk_add_test_cxx(vtkIOXMLCxxTests tests
  TestXMLReaderBadUniformGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badUniformGridData.xml}"
)

vtk_test_cxx_executable(vtkIOXMLCxxTests tests)

add_executable(TestXMLCInterface MACOSX_BUNDLE TestXMLCInterface.c)
target_link_libraries(TestXMLCInterface PRIVATE VTK::IOXML)
add_test(NAME VTK::IOXML-TestXMLCInterface COMMAND TestXMLCInterface)

# When using 32-bit IDs, this test will not pass since the ids
# cannot be read in correctly, so we mark it as skipped if that's the
# reason why the test fails.
set_property(
  TEST VTK::IOXMLCxx-TestXMLLegacyFileReadIdTypeArrays
  APPEND
  PROPERTY SKIP_REGULAR_EXPRESSION "IdType tag has been ignored")

# Writing a large file during this test takes extra time, especially when other tests are running simultaneously.
if(NOT VTK_TEST_TIMEOUT_TestXMLLargeUnstructuredGrid)
  set(VTK_TEST_TIMEOUT_TestXMLLargeUnstructuredGrid 1000)
endif()
set_tests_properties(VTK::IOXMLCxx-TestXMLLargeUnstructuredGrid
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestXMLLargeUnstructuredGrid})
