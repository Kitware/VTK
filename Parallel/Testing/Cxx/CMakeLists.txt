LINK_LIBRARIES (
vtkParallel
vtkRendering
vtkIO
vtkGraphics
vtkImaging
vtkFiltering 
vtkCommon 
)

IF (HAVE_PNG)
  LINK_LIBRARIES( ${PNG_LIBRARY} )

  IF (VTK_USE_HYBRID)
    ADD_EXECUTABLE(ParallelIsoTest ParallelIso)
    TARGET_LINK_LIBRARIES(ParallelIsoTest vtkPatented vtkHybrid)
  ENDIF (VTK_USE_HYBRID)

  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )

  IF (CMAKE_HAS_X)
    LINK_LIBRARIES(-lXt)
    LINK_LIBRARIES(${CMAKE_X_LIBS})
  ENDIF (CMAKE_HAS_X)

  IF (VTK_USE_MPI)
    IF (MPI_LIBRARY)
      LINK_LIBRARIES (${MPI_LIBRARY})
    ENDIF (MPI_LIBRARY)
    IF (MPI_INCLUDE_PATH) 
      INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
    ENDIF(MPI_INCLUDE_PATH)
  ENDIF (VTK_USE_MPI)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_USE_HYBRID)
      IF (VTK_USE_MPI)
        IF (MPIRUN)
          ADD_TEST(ParallelIsoMPI-image ${MPIRUN} ${MPI_PREFLAGS}
                   ${CXX_TEST_PATH}/ParallelIsoTest
                   -D ${VTK_DATA_ROOT}
                   -V Baseline/Parallel/ParallelIsoTest.png
                   ${MPI_POSTFLAGS})
        ENDIF(MPIRUN)
      ELSE (VTK_USE_MPI)
        ADD_TEST(ParallelIso-image ${CXX_TEST_PATH}/ParallelIsoTest
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/ParallelIsoTest.png)
      ENDIF (VTK_USE_MPI)
    ENDIF (VTK_USE_HYBRID)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    IF (VTK_USE_HYBRID)
      IF (VTK_USE_MPI)
        IF (MPIRUN)
          ADD_TEST(ParallelIsoMPI ${MPIRUN} ${MPI_PREFLAGS}
                   ${CXX_TEST_PATH}/ParallelIsoTest ${MPI_POSTFLAGS})
        ENDIF(MPIRUN)
      ELSE (VTK_USE_MPI)
        ADD_TEST(ParallelIso ${CXX_TEST_PATH}/ParallelIsoTest)
      ENDIF (VTK_USE_MPI)
    ENDIF (VTK_USE_HYBRID)
  ENDIF (VTK_DATA_ROOT)

ENDIF (HAVE_PNG)



