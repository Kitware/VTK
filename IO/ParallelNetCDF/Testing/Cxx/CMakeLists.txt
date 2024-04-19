vtk_add_test_cxx(vtkIOParallelNetCDFCxxTests tests
  TestPNetCDFPOPReader.cxx
  )
vtk_test_cxx_executable(vtkIOParallelNetCDFCxxTests tests)

if(VTK_USE_LARGE_DATA AND TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkIOParallelNetCDFCxxTests-MPI tests
    TESTING_DATA
    PSLACReaderLinear.cxx
    PSLACReaderQuadratic.cxx
    )
  vtk_test_cxx_executable(vtkIOParallelNetCDFCxxTests-MPI tests)
endif()
