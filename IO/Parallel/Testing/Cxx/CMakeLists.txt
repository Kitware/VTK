vtk_add_test_cxx(vtkIOParallelCxxTests tests
  TestPOpenFOAMReader.cxx
  TestBigEndianPlot3D.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOParallelCxxTests tests)

if(VTK_USE_LARGE_DATA AND TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkIOParallelCxxTests-MPI tests
    TESTING_DATA
    PSLACReaderLinear.cxx
    PSLACReaderQuadratic.cxx
    )
  vtk_test_cxx_executable(vtkIOParallelCxxTests-MPI tests)
endif()
