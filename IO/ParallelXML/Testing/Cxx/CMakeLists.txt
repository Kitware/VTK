# note, to enable testing the vtkParallelMPI module should be enabled
if (TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkIOParallelXMLCxxTests tests
    TESTING_DATA NO_VALID
    TestParallelPartitionedDataSetIO.cxx
    TestParallelUnstructuredGridIO.cxx
    TestXMLReaderChangingBlocksOverTime.cxx)
else()
  vtk_add_test_cxx(vtkIOParallelXMLCxxTests tests
    NO_VALID
    TestParallelPartitionedDataSetIO.cxx
    TestParallelUnstructuredGridIO.cxx
    TestXMLReaderChangingBlocksOverTime.cxx)
endif()

vtk_test_cxx_executable(vtkIOParallelXMLCxxTests tests)
