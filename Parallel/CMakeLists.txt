SOURCE_FILES ( Parallel_SRCS
vtkCommunicator 
vtkCutMaterial 
vtkExtractPolyDataPiece 
vtkExtractUnstructuredGridPiece 
vtkGetRemoteGhostCells 
vtkImageComposite 
vtkInputPort 
vtkMemoryLimitImageDataStreamer 
vtkMultiProcessController 
vtkOutputPort 
vtkParallelFactory 
vtkPieceScalars 
vtkPipelineSize 
vtkPImageWriter 
vtkPPolyDataNormals 
vtkPSphereSource 
vtkRTAnalyticSource 
vtkSharedMemoryCommunicator 
vtkThreadedController 
vtkTreeComposite 
vtkRemoveGhostCells 
)

ABSTRACT_FILES( vtkCommunicator )

IF (VTK_USE_MPI)
  INCLUDE (${CMAKE_SOURCE_DIR}/CMake/Modules/FindMPI.cmake)
  IF (MPI_LIB_PATH)
    SOURCE_FILES ( Parallel_SRCS
      vtkMPICommunicator 
      vtkMPIController 
      vtkMPIGroup)
  LINK_LIBRARIES (${MPI_LIBRARY})
  LINK_DIRECTORIES (${MPI_LIB_PATH})
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF (MPI_LIB_PATH)
ENDIF (VTK_USE_MPI)

ADD_LIBRARY(vtkParallel Parallel_SRCS)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkParallelTCL ParallelTCL_SRCS Parallel_SRCS)
  ADD_LIBRARY(vtkParallelTCL ParallelTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkParallelTCL vtkIOTCL vtkRenderingTCL
                         vtkGraphicsTCL vtkImagingTCL vtkFilteringTCL 
                         vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)


# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  ADD_LIBRARY(vtkParallelPython ParallelPython_SRCS)
  VTK_WRAP_PYTHON(vtkParallelPython ParallelPython_SRCS Parallel_SRCS)
  TARGET_LINK_LIBRARIES (vtkParallelPython 
                         vtkRenderingPython
                         vtkGraphicsPython
                         vtkImagingPython
                         vtkFilteringPython
                         vtkCommonPython)
  TARGET_LINK_LIBRARIES (vtkParallelPython 
                         debug ${PY_DEBUG_LIB_PATH} optimized ${PY_LIB_PATH}))
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkParallelJava ParallelJava_SRCS Parallel_SRCS)
  ADD_LIBRARY(vtkParallelJava ParallelJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkParallelJava vtkCommonJava vtkFilteringJava
                         vtkImagingJava vtkGraphicsJava vtkRenderingJava
                         vtkIOJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkParallel
)


