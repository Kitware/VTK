SET(KIT Parallel)
SET(UKIT PARALLEL)
SET(KIT_TCL_LIBS vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkIOJava)
SET(KIT_LIBS vtkIO)
IF(VTK_USE_RENDERING)
  SET(KIT_TCL_LIBS vtkRenderingTCL ${KIT_TCL_LIBS})
  SET(KIT_PYTHON_LIBS vtkRenderingPythonD ${KIT_PYTHON_LIBS})
  SET(KIT_JAVA_LIBS vtkRenderingJava ${KIT_JAVA_LIBS})
  SET(KIT_LIBS vtkRendering ${KIT_LIBS})
ENDIF(VTK_USE_RENDERING)
IF(VTK_HAS_EXODUS)
  SET(KIT_LIBS ${KIT_LIBS} vtkexoIIc)
ENDIF(VTK_HAS_EXODUS)

SET ( Kit_SRCS
vtkBranchExtentTranslator.cxx
vtkCachingInterpolatedVelocityField.cxx
vtkCollectGraph.cxx
vtkCollectPolyData.cxx
vtkCollectTable.cxx
vtkCommunicator.cxx
vtkCompositer.cxx
vtkCompressCompositer.cxx
vtkCutMaterial.cxx
vtkDistributedDataFilter.cxx
vtkDistributedStreamTracer.cxx
vtkDummyCommunicator.cxx
vtkDummyController.cxx
vtkDuplicatePolyData.cxx
vtkEnSightWriter.cxx
vtkExtractCTHPart.cxx
vtkExtractPiece.cxx
vtkExtractPolyDataPiece.cxx
vtkExtractUnstructuredGridPiece.cxx
vtkExtractUserDefinedPiece.cxx
vtkMemoryLimitImageDataStreamer.cxx
vtkMultiProcessController.cxx
vtkMultiProcessStream.cxx
vtkParallelFactory.cxx
vtkPassThroughFilter.cxx
vtkPCellDataToPointData.cxx
vtkPChacoReader.cxx
vtkPDataSetReader.cxx
vtkPDataSetWriter.cxx
vtkPExtractArraysOverTime.cxx
vtkPieceRequestFilter.cxx
vtkPieceScalars.cxx
vtkPImageWriter.cxx
vtkPKdTree.cxx
vtkPLinearExtrusionFilter.cxx
vtkPOPReader.cxx
vtkPOutlineCornerFilter.cxx
vtkPOutlineFilter.cxx
vtkPPolyDataNormals.cxx
vtkPProbeFilter.cxx
vtkProcessGroup.cxx
vtkProcessIdScalars.cxx
vtkPReflectionFilter.cxx
vtkPSphereSource.cxx
vtkPStreamTracer.cxx
vtkRectilinearGridOutlineFilter.cxx
vtkSocketCommunicator.cxx
vtkSocketController.cxx
vtkSubCommunicator.cxx
vtkSubGroup.cxx
vtkTemporalFractal.cxx
vtkTemporalInterpolatedVelocityField.cxx
vtkTemporalStreamTracer.cxx
vtkTransmitImageDataPiece.cxx
vtkTransmitPolyDataPiece.cxx
vtkTransmitRectilinearGridPiece.cxx
vtkTransmitStructuredGridPiece.cxx
vtkTransmitUnstructuredGridPiece.cxx
vtkXMLPHierarchicalBoxDataWriter.cxx
vtkXMLPMultiBlockDataWriter.cxx
)

IF(VTK_USE_RENDERING)
  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkCompositeRenderManager.cxx
    vtkPipelineSize.cxx
    vtkParallelRenderManager.cxx
    vtkTreeCompositer.cxx
    )
ENDIF(VTK_USE_RENDERING)

IF(VTK_HAS_EXODUS)
  IF(NOT WIN32)
    SET(Kit_SRCS ${Kit_SRCS}
      vtkExodusIIWriter.cxx
    )
  ENDIF (NOT WIN32)
ENDIF(VTK_HAS_EXODUS)

SET_SOURCE_FILES_PROPERTIES(
vtkCommunicator 
vtkMultiProcessController
vtkMultiProcessStream
vtkPStreamTracer
ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
vtkMultiProcessStream
WRAP_EXCLUDE
)

IF(VTK_USE_RENDERING)
  SET_SOURCE_FILES_PROPERTIES(
  vtkParallelRenderManager
  ABSTRACT
  )
ENDIF(VTK_USE_RENDERING)

IF (VTK_USE_MPI)
  INCLUDE (FindMPI)
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

IF (VTK_USE_PARALLEL_BGL)
  SET(KIT_LIBS ${KIT_LIBS} boost_graph_mpi ${Boost_LIBRARIES})
  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkPBGLBreadthFirstSearch.cxx
    vtkPBGLCollapseGraph.cxx
    vtkPBGLCollapseParallelEdges.cxx
    vtkPBGLCollectGraph.cxx
    vtkPBGLConnectedComponents.cxx
    vtkPBGLDistributedGraphHelper.cxx
    vtkPBGLGraphSQLReader.cxx
    vtkPBGLMinimumSpanningTree.cxx
    vtkPBGLRMATGraphSource.cxx
    vtkPBGLRandomGraphSource.cxx
    vtkPBGLShortestPaths.cxx
    vtkPBGLVertexColoring.cxx
    )
ENDIF (VTK_USE_PARALLEL_BGL)

IF (VTK_USE_MPI) 
  IF (MPI_LIBRARIES)
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_LIBRARIES}")
  ELSE (MPI_LIBRARIES)
    IF (MPI_LIBRARY) 
      SET(KIT_LIBS ${KIT_LIBS} "${MPI_LIBRARY}")   
    ELSE (MPI_LIBRARY)   
      MESSAGE("Could not find the required MPI libraries")
    ENDIF (MPI_LIBRARY)   
    IF (MPI_EXTRA_LIBRARY)   
      SET(KIT_LIBS ${KIT_LIBS} "${MPI_EXTRA_LIBRARY}")   
    ENDIF (MPI_EXTRA_LIBRARY) 
  ENDIF (MPI_LIBRARIES)
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

IF (VTK_USE_PARALLEL_BGL)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/vtkPBGLGraphAdapter.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
      COMPONENT Development
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF (VTK_USE_PARALLEL_BGL)
