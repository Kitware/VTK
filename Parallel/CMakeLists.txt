SET(KIT Parallel)
SET(UKIT PARALLEL)
SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL )
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO)
IF(VTK_HAS_EXODUS)
  SET(KIT_LIBS ${KIT_LIBS} vtkexoIIc)
ENDIF(VTK_HAS_EXODUS)
IF (WIN32)
  IF (NOT BORLAND)
    SET(KIT_LIBS ${KIT_LIBS} wsock32)
  ENDIF (NOT BORLAND)
ENDIF (WIN32)

SET ( Kit_SRCS
vtkDuplicatePolyData.cxx
vtkBranchExtentTranslator.cxx
vtkCollectPolyData.cxx
vtkCommunicator.cxx
vtkCompositer.cxx
vtkCompositeRenderManager.cxx
vtkCompressCompositer.cxx
vtkCutMaterial.cxx
vtkDistributedDataFilter.cxx
vtkDistributedStreamTracer.cxx
vtkDummyController.cxx
vtkEnSightWriter.cxx
vtkExtractCTHPart.cxx
vtkExtractPolyDataPiece.cxx
vtkExtractUnstructuredGridPiece.cxx
vtkExtractUserDefinedPiece.cxx
vtkPKdTree.cxx
vtkMemoryLimitImageDataStreamer.cxx
vtkMultiProcessController.cxx
vtkParallelRenderManager.cxx
vtkPassThroughFilter.cxx
vtkPCellDataToPointData.cxx
vtkPChacoReader.cxx
vtkPDataSetReader.cxx
vtkPDataSetWriter.cxx
vtkPImageWriter.cxx
vtkPLinearExtrusionFilter.cxx
vtkPOPReader.cxx
vtkPOutlineFilter.cxx
vtkPOutlineCornerFilter.cxx
vtkPPolyDataNormals.cxx
vtkPProbeFilter.cxx
vtkPSphereSource.cxx
vtkPStreamTracer.cxx
vtkParallelFactory.cxx
vtkPieceScalars.cxx
vtkPipelineSize.cxx
vtkProcessIdScalars.cxx
vtkRTAnalyticSource.cxx
vtkRectilinearGridOutlineFilter.cxx
vtkSocketCommunicator.cxx
vtkSocketController.cxx
vtkSubGroup.cxx
vtkTransmitPolyDataPiece.cxx
vtkTransmitUnstructuredGridPiece.cxx
vtkTreeCompositer.cxx
)

IF(VTK_HAS_EXODUS)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkExodusIIWriter.cxx
    )
ENDIF(VTK_HAS_EXODUS)

SET_SOURCE_FILES_PROPERTIES(
vtkCommunicator 
vtkMultiProcessController
vtkParallelRenderManager
vtkPStreamTracer
ABSTRACT
)

IF (VTK_USE_MPI)
  INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkMPICommunicator.cxx
    vtkMPIController.cxx 
    vtkMPIGroup.cxx)
ENDIF (VTK_USE_MPI)

IF (VTK_USE_MPI) 
  IF (MPI_LIBRARY) 
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_LIBRARY}")   
  ELSE (MPI_LIBRARY)   
    MESSAGE("Could not find the required MPI libraries")
  ENDIF (MPI_LIBRARY)   
  IF (MPI_EXTRA_LIBRARY)   
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_EXTRA_LIBRARY}")   
  ENDIF (MPI_EXTRA_LIBRARY) 
ENDIF (VTK_USE_MPI)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_SOURCE_DIR}/CMake/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

IF (VTK_USE_MPI)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} "vtkMPI\\.h")
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF (VTK_USE_MPI)
