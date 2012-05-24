include_directories("${MPI_CXX_INCLUDE_PATH}")
if (MPI_LIBRARY)
  set(MPI_LIBRARIES ${MPI_LIBRARY})
endif (MPI_LIBRARY)
if (MPI_EXTRA_LIBRARY)
  set(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")
endif (MPI_EXTRA_LIBRARY)

# Add parallel tests only if PARALLEL and MPI are turned ON
# These tests are kept separate, because they must be run with mpirun or mpiexec

# TestRandomPMomentStatisticsMPI:
# MPI test of parallel descriptive, correlative, multicorrelative and PCA statistics
add_executable(TestRandomPMomentStatisticsMPI TestRandomPMomentStatisticsMPI.cxx)
target_link_libraries(TestRandomPMomentStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

# TestRandomPContingencyStatisticsMPI:
# MPI test of parallel contingency statistics
add_executable(TestRandomPContingencyStatisticsMPI TestRandomPContingencyStatisticsMPI.cxx)
target_link_libraries(TestRandomPContingencyStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

# TestRandomPOrderStatisticsMPI:
# MPI test of parallel order statistics
add_executable(TestRandomPOrderStatisticsMPI TestRandomPOrderStatisticsMPI.cxx)
target_link_libraries(TestRandomPOrderStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

# TestParallelKMeansStatisticsMPI:
# MPI test of parallel KMeans statistics
add_executable(TestRandomPKMeansStatisticsMPI TestRandomPKMeansStatisticsMPI.cxx)
target_link_libraries(TestRandomPKMeansStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

if (VTK_MPIRUN_EXE)
  add_test(TestRandomPMomentStatisticsMPI
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
    ${VTK_MPI_PREFLAGS}
    ${CXX_TEST_PATH}/${CXX_TEST_CONFIG}/TestRandomPMomentStatisticsMPI
    ${VTK_MPI_POSTFLAGS}
    )

  add_test(TestRandomPContingencyStatisticsMPI
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
    ${VTK_MPI_PREFLAGS}
    ${CXX_TEST_PATH}/${CXX_TEST_CONFIG}/TestRandomPContingencyStatisticsMPI
    ${VTK_MPI_POSTFLAGS}
    )

  add_test(TestRandomPOrderStatisticsMPI
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
    ${VTK_MPI_PREFLAGS}
    ${CXX_TEST_PATH}/${CXX_TEST_CONFIG}/TestRandomPOrderStatisticsMPI
    ${VTK_MPI_POSTFLAGS}
    )

  add_test(TestRandomPKMeansStatisticsMPI
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
    ${VTK_MPI_PREFLAGS}
    ${CXX_TEST_PATH}/${CXX_TEST_CONFIG}/TestRandomPKMeansStatisticsMPI
    ${VTK_MPI_POSTFLAGS}
    )
endif (VTK_MPIRUN_EXE)
