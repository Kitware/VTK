if (TARGET VTK::conduit)
  set(CxxTests TestFidesConduitBasic.cxx)

  # The ARGS must be set before the test addition macros
  set(TestFidesConduitBasic_ARGS "${CMAKE_CURRENT_BINARY_DIR}")

  if (TARGET VTK::ParallelMPI)
    # Run in parallel if MPI is enabled
    vtk_add_test_mpi(vtkIOFidesCxxTests-MPI mpi_tests
      NO_VALID
      ${CxxTests}
    )

    vtk_test_cxx_executable(vtkIOFidesCxxTests-MPI mpi_tests)
    target_link_libraries(vtkIOFidesCxxTests-MPI PRIVATE VTK::conduit VTK::ParallelMPI)
    target_compile_definitions(vtkIOFidesCxxTests-MPI PRIVATE TEST_HAS_MPI=1)

  else()
    # Run in serial if MPI is not enabled
    vtk_add_test_cxx(vtkIOFidesCxxTests tests
      NO_VALID
      ${CxxTests}
    )

    vtk_test_cxx_executable(vtkIOFidesCxxTests tests)
    target_link_libraries(vtkIOFidesCxxTests PRIVATE VTK::conduit)
    target_compile_definitions(vtkIOFidesCxxTests PRIVATE TEST_HAS_MPI=0)
  endif()
else()
  vtk_module_test_data(
    Data/vtk-uns-grid-2.json
    Data/tris-blocks-time.bp
    Data/tris-blocks-time.bp.dir/,REGEX:.*)

  set(NoConduitCxxTests TestFidesReaderNoConduit.cxx)

  vtk_add_test_cxx(vtkIOFidesCxxTests-NoConduit no_conduit_tests
    NO_VALID
    ${NoConduitCxxTests}
  )

  vtk_test_cxx_executable(vtkIOFidesCxxTests-NoConduit no_conduit_tests)
endif()
