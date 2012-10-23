INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j|Task|Pipe).*$")

IF (VTK_USE_MPI)
  find_package(MPI REQUIRED)
  INCLUDE_DIRECTORIES(${MPI_C_INCLUDE_PATH})

  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")

  IF (VTK_USE_RENDERING)
    ADD_EXECUTABLE(ParallelIso ParallelIso.cxx)
    TARGET_LINK_LIBRARIES(ParallelIso vtkParallel vtkHybrid
      ${MPI_C_LIBRARIES})
    if (MPI_CXX_LIBRARIES)
      TARGET_LINK_LIBRARIES(ParallelIso ${MPI_CXX_LIBRARIES})
    endif()
    IF(APPLE AND VTK_USE_CARBON)
      TARGET_LINK_LIBRARIES(ParallelIso "-framework Carbon")
    ENDIF(APPLE AND VTK_USE_CARBON)
  ENDIF (VTK_USE_RENDERING)

  SET(TaskPara_SRCS TaskParallelism.cxx task1.cxx task2.cxx)

  ADD_EXECUTABLE(TaskParallelism          ${TaskPara_SRCS})
  TARGET_LINK_LIBRARIES (TaskParallelism vtkParallel)
  IF(APPLE AND VTK_USE_CARBON)
   TARGET_LINK_LIBRARIES(TaskParallelism "-framework Carbon")
  ENDIF(APPLE AND VTK_USE_CARBON)

#  ADD_EXECUTABLE(SimpleBenchmark SimpleBenchmark.cxx)
#  TARGET_LINK_LIBRARIES(SimpleBenchmark vtkParallel ${MPI_LIBRARIES} )
ENDIF (VTK_USE_MPI)
