cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(ParallelProcessingGeneric)

find_package(VTK
  COMPONENTS
    IOImage
    ImagingCore
    ImagingGeneral
    FiltersSources
    ParallelMPI
    RenderingCore
    RenderingParallel
    TestingCore
    TestingRendering
    mpi)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(ParallelIso ParallelIso.cxx)
target_link_libraries(ParallelIso
  PRIVATE
    ${VTK_LIBRARIES})

set(TaskPara_SRCS
  TaskParallelism.cxx
  task1.cxx
  task2.cxx)

add_executable(TaskParallelism
  ${TaskPara_SRCS})
target_link_libraries(TaskParallelism
  PRIVATE
    ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS ParallelIso
          TaskParallelism
  MODULES ${VTK_LIBRARIES})
