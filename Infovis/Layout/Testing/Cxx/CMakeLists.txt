vtk_add_test_cxx(Infovis
  # TestAssignCoordinates.cxx # todo (unsatisfied deps)
  # TestChacoGraphReader.cxx # todo (unsatisfied deps)
  TestCirclePackLayoutStrategy.cxx
  # TestCosmicTreeLayoutStrategy.cxx # todo (unsatisfied deps)
  TestGraphLayoutStrategy.cxx
  TestIncrementalForceLayout.cxx
  TestTreeMapLayoutStrategy.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
