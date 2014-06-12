include(vtkMPI)

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  PBGLNamedVertexGraph.cxx
  PBGLRandomGraph.cxx
  TestPBGLAlgorithms.cxx
  TestPBGLCollapseGraph.cxx
  TestPBGLCollectGraph.cxx
  TestPBGLEdgesPedigrees.cxx
  TestPBGLGraphSQLReader.cxx
  TestPBGLGraphSQLReaderFile.cxx
  TestPBGLPedigrees.cxx
  TestPBGLPipeline.cxx
  TestPRMATGraphSource.cxx
  TestPRandomGraphSource.cxx
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
