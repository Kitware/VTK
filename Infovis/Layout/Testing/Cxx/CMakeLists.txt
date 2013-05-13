vtk_add_test_cxx(Infovis
  # TestAssignCoordinates.cxx # todo (unsatisfied deps)
  # TestChacoGraphReader.cxx # todo (unsatisfied deps)
  TestCirclePackLayoutStrategy.cxx
  # TestCosmicTreeLayoutStrategy.cxx # todo (unsatisfied deps)
  TestGraphLayoutStrategy.cxx,NO_VALID
  TestIncrementalForceLayout.cxx,NO_VALID
  TestTreeMapLayoutStrategy.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
