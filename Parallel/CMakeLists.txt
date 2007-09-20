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
vtkCollectGraph.cxx
vtkCollectPolyData.cxx
vtkCollectTable.cxx
vtkCommunicator.cxx
vtkCompositer.cxx
vtkCompositeRenderManager.cxx
vtkCompressCompositer.cxx
vtkCutMaterial.cxx
vtkDistributedDataFilter.cxx
vtkDistributedStreamTracer.cxx
vtkDummyCommunicator.cxx
vtkDummyController.cxx
vtkEnSightWriter.cxx
vtkExtractCTHPart.cxx
vtkExtractPolyDataPiece.cxx
vtkExtractUnstructuredGridPiece.cxx
vtkExtractUserDefinedPiece.cxx
vtkMultiGroupDataExtractPiece.cxx
vtkPKdTree.cxx
vtkMemoryLimitImageDataStreamer.cxx
vtkMultiProcessController.cxx
vtkParallelRenderManager.cxx
vtkPassThroughFilter.cxx
vtkPCellDataToPointData.cxx
vtkPChacoReader.cxx
vtkPDataSetReader.cxx
vtkPDataSetWriter.cxx
vtkPExtractArraysOverTime.cxx
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
vtkPieceRequestFilter.cxx
vtkPieceScalars.cxx
vtkPipelineSize.cxx
vtkProcessIdScalars.cxx
vtkProcessGroup.cxx
vtkRTAnalyticSource.cxx
vtkRectilinearGridOutlineFilter.cxx
vtkSubCommunicator.cxx
vtkSubGroup.cxx
vtkTemporalFractal.cxx
vtkTemporalInterpolatedVelocityField.cxx
vtkTemporalStreamTracer.cxx
vtkTransmitImageDataPiece.cxx
vtkTransmitRectilinearGridPiece.cxx
vtkTransmitStructuredGridPiece.cxx
vtkTransmitPolyDataPiece.cxx
vtkTransmitUnstructuredGridPiece.cxx
vtkTreeCompositer.cxx
vtkXMLPMultiGroupDataWriter.cxx
)

# this is only for Cray Xt3/Catamount, not sure it is the best way to do it, Alex
IF(HAVE_SOCKETS)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkClientSocket.cxx
    vtkServerSocket.cxx
    vtkSocket.cxx
    vtkSocketCollection.cxx
    vtkSocketCommunicator.cxx
    vtkSocketController.cxx )
ENDIF(HAVE_SOCKETS)


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
vtkSocket
ABSTRACT
)

IF (VTK_USE_MPI)
  INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkMPICommunicator.cxx
    vtkMPIController.cxx 
    )
  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ENDIF (VTK_USE_MPI)

IF (NOT VTK_LEGACY_REMOVE)
  SET(Kit_Deprecated_SRCS
    )
  IF (VTK_USE_MPI)
    SET(Kit_Deprecated_SRCS ${Kit_Deprecated_SRCS}
      vtkMPIGroup.cxx
      )
  ENDIF (VTK_USE_MPI)
  SET (Kit_SRCS ${Kit_SRCS} ${Kit_Deprecated_SRCS})
  IF (CMAKE_COMPILER_IS_GNUCXX)
    SET_SOURCE_FILES_PROPERTIES(${Kit_Deprecated_SRCS}
      PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations
      )
  ENDIF (CMAKE_COMPILER_IS_GNUCXX)
ENDIF (NOT VTK_LEGACY_REMOVE)

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
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

IF (VTK_USE_MPI)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/vtkMPI.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
      COMPONENT Development
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF (VTK_USE_MPI)

