INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j|Task|Pipe).*$")

include(vtkMPI)

find_package(VTK COMPONENTS
  vtkImagingCore
  vtkImagingGeneral
  vtkFiltersCore
  vtkFiltersSources
  vtkParallelMPI
  vtkRenderingCore
  vtkRenderingParallel
  vtkTestingCore
  vtkTestingRendering
)
include(${VTK_USE_FILE})

# Needed for mpich 2
ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")

ADD_EXECUTABLE(ParallelIso ParallelIso.cxx)
TARGET_LINK_LIBRARIES(ParallelIso LINK_PRIVATE ${VTK_LIBRARIES})
VTK_MPI_LINK(ParallelIso)

SET(TaskPara_SRCS TaskParallelism.cxx task1.cxx task2.cxx)

ADD_EXECUTABLE(TaskParallelism          ${TaskPara_SRCS})
TARGET_LINK_LIBRARIES (TaskParallelism ${VTK_LIBRARIES})

#  ADD_EXECUTABLE(SimpleBenchmark SimpleBenchmark.cxx)
#  TARGET_LINK_LIBRARIES(SimpleBenchmark vtkParallel ${MPI_LIBRARIES} )
