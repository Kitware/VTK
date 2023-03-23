vtk_add_test_cxx(vtkIOLegacyCxxTests tests
  TestLegacyArrayMetaData.cxx,NO_VALID
  TestLegacyCompositeDataReaderWriter.cxx,NO_VALID
  TestLegacyGhostCellsImport.cxx
  TestLegacyMappedUnstructuredGrid.cxx,NO_DATA,NO_VALID
  TestLegacyPartitionedDataSetCollectionReaderWriter.cxx,NO_DATA,NO_VALID
  TestLegacyPartitionedDataSetReaderWriter.cxx,NO_DATA,NO_VALID
  TestLegacyPolyDataReaderErrorCodePath.cxx, NO_VALID
  TestLegacyDataSetWriterSetFileVersion.cxx,NO_DATA,NO_VALID
  )

vtk_test_cxx_executable(vtkIOLegacyCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
