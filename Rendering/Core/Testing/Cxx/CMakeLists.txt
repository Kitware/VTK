vtk_add_test_cxx(
  TestAssemblyBounds.cxx,NO_VALID
  TestPickingManager.cxx,NO_VALID
  TestSplitViewportStereoHorizontal.cxx
  # otherLookupTable.cxx             # fixme
  # otherLookupTableWithEnabling.cxx # fixme
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
