vtk_add_test_cxx(Rendering
  TestPickingManager.cxx
  TestSplitViewportStereoHorizontal.cxx
  # otherLookupTable.cxx             # fixme
  # otherLookupTableWithEnabling.cxx # fixme
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
