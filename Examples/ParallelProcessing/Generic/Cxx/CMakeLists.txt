INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j|Task|Pipe).*$")

IF (VTK_USE_MPI)
  IF (VTK_USE_RENDERING)
    ADD_EXECUTABLE(ParallelIso ParallelIso.cxx)
    TARGET_LINK_LIBRARIES(ParallelIso vtkParallel vtkHybrid)
    IF(APPLE)
      TARGET_LINK_LIBRARIES(ParallelIso "-framework Carbon")
    ENDIF(APPLE)
  ENDIF (VTK_USE_RENDERING)

  SET(TaskPara_SRCS TaskParallelism.cxx task1.cxx task2.cxx)

  ADD_EXECUTABLE(TaskParallelism          ${TaskPara_SRCS})

  TARGET_LINK_LIBRARIES (TaskParallelism          vtkParallel)
  IF(APPLE)
   TARGET_LINK_LIBRARIES(TaskParallelism "-framework Carbon")
  ENDIF(APPLE)
ENDIF (VTK_USE_MPI)
