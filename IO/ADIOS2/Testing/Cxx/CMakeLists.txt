find_package(ADIOS2
             2.4 REQUIRED)

if (ADIOS2_HAVE_MPI)
  set(TestADIOS2BPReaderSingleTimeStep_NUMPROCS 2)
# For now vtkMultiBlockVolumeMapper does not support rendering in parallel
  set(TestADIOS2BPReaderMultiTimeSteps_NUMPROCS 2)
  set(TestADIOS2BPReaderMultiTimeSteps2D_NUMPROCS 2)
  vtk_add_test_mpi(vtkIOADIOS2CxxTests-MPI mpiTests
    TESTING_DATA
    TestADIOS2BPReaderMPISingleTimeStep.cxx
    TestADIOS2BPReaderMPIMultiTimeSteps3D.cxx,NO_VALID
    TestADIOS2BPReaderMPIMultiTimeSteps2D.cxx
    )
  vtk_test_cxx_executable(vtkIOADIOS2CxxTests-MPI mpiTests)

  # VAR tests
  vtk_add_test_cxx(vtkIOADIOS2VARCxxTests tests
    NO_DATA NO_VALID NO_OUTPUT
    UnitTestIOADIOS2VAR.cxx
    TestIOADIOS2VAR_VTI3D.cxx
    TestIOADIOS2VAR_VTI3DRendering.cxx
    TestIOADIOS2VAR_VTU3D.cxx
    TestIOADIOS2VAR_VTU3DRendering.cxx
  )

  vtk_test_cxx_executable(vtkIOADIOS2VARCxxTests tests)
  target_link_libraries(vtkIOADIOS2VARCxxTests
    PUBLIC
      adios2::adios2 )
else()
  vtk_add_test_cxx(vtkIOADIOS2CxxTests tests
    TESTING_DATA
    TestADIOS2BPReaderSingleTimeStep.cxx
    TestADIOS2BPReaderMultiTimeSteps3D.cxx
    TestADIOS2BPReaderMultiTimeSteps2D.cxx)

  vtk_test_cxx_executable(vtkIOADIOS2CxxTests tests)
endif()
