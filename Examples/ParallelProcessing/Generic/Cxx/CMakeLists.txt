INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j|Task|Pipe).*$")

IF (VTK_USE_HYBRID)
  ADD_EXECUTABLE(ParallelIso ParallelIso.cxx)
  TARGET_LINK_LIBRARIES(ParallelIso vtkParallel vtkHybrid)
ENDIF (VTK_USE_HYBRID)

SET(TaskPara_SRCS TaskParallelism.cxx task1.cxx task2.cxx)
SET(TaskParaPorts_SRCS TaskParallelismWithPorts.cxx task3.cxx task4.cxx)
SET(PipePara_SRCS PipelineParallelism.cxx pipe1.cxx pipe2.cxx)

ADD_EXECUTABLE(TaskParallelism          ${TaskPara_SRCS})
ADD_EXECUTABLE(TaskParallelismWithPorts ${TaskParaPorts_SRCS})
ADD_EXECUTABLE(PipelineParallelism      ${PipePara_SRCS})

TARGET_LINK_LIBRARIES (TaskParallelism          vtkParallel)
TARGET_LINK_LIBRARIES (TaskParallelismWithPorts vtkParallel)
TARGET_LINK_LIBRARIES (PipelineParallelism      vtkParallel)
