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

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkParallel
)


