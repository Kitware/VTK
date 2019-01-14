vtk_add_test_cxx(vtkIOLegacyCxxTests tests
  TestLegacyCompositeDataReaderWriter.cxx,NO_VALID
  TestLegacyGhostCellsImport.cxx
  TestLegacyArrayMetaData.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOLegacyCxxTests tests
    RENDERING_FACTORY
    )
