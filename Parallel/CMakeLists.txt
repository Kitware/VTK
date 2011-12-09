SET(KIT Parallel)
SET(UKIT PARALLEL)
SET(KIT_TCL_LIBS vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkIOJava)
SET(KIT_INTERFACE_LIBRARIES vtkIO)
SET(KIT_LIBS VPIC Cosmo LSDyna)
IF(VTK_USE_RENDERING)
  SET(KIT_TCL_LIBS vtkRenderingTCL ${KIT_TCL_LIBS})
  SET(KIT_PYTHON_LIBS vtkRenderingPythonD ${KIT_PYTHON_LIBS})
  SET(KIT_JAVA_LIBS vtkRenderingJava ${KIT_JAVA_LIBS})
  SET(KIT_INTERFACE_LIBRARIES vtkRendering ${KIT_INTERFACE_LIBRARIES})
  SET(KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY})
  IF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
    SET (KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY} )
  ENDIF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
ELSE(VTK_USE_RENDERING)
  SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} vtkGraphicsPythonD)
  SET(KIT_LIBS ${KIT_LIBS} vtkGraphics vtkImaging)
ENDIF(VTK_USE_RENDERING)
IF(VTK_HAS_EXODUS)
  SET(KIT_LIBS ${KIT_LIBS} vtkexoIIc)
ENDIF(VTK_HAS_EXODUS)

SET ( Kit_SRCS
vtkBranchExtentTranslator.cxx
vtkCachingInterpolatedVelocityField.cxx
vtkClientServerSynchronizedRenderers.cxx
vtkCollectGraph.cxx
vtkCollectPolyData.cxx
vtkCollectTable.cxx
vtkCommunicator.cxx
vtkCompositedSynchronizedRenderers.cxx
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
vtkMPIImageReader.cxx
vtkMultiProcessController.cxx
vtkMultiProcessStream.cxx
vtkParallelFactory.cxx
vtkPassThroughFilter.cxx
vtkPCellDataToPointData.cxx
vtkPChacoReader.cxx
vtkPCosmoHaloFinder.cxx
vtkPCosmoReader.cxx
vtkPDataSetReader.cxx
vtkPDataSetWriter.cxx
vtkPExtractArraysOverTime.cxx
vtkPHardwareSelector.cxx
vtkPieceRequestFilter.cxx
vtkPieceScalars.cxx
vtkPKdTree.cxx
vtkPLinearExtrusionFilter.cxx
vtkPMaskPoints.cxx
vtkPNrrdReader.cxx
vtkPOPReader.cxx
vtkPOpenFOAMReader.cxx
vtkPOutlineCornerFilter.cxx
vtkPOutlineFilter.cxx
vtkPPolyDataNormals.cxx
vtkPProbeFilter.cxx
vtkProcess.cxx
vtkProcessGroup.cxx
vtkProcessIdScalars.cxx
vtkPLSDynaReader.cxx
vtkPReflectionFilter.cxx
vtkPSLACReader.cxx
vtkPSphereSource.cxx
vtkPStreamTracer.cxx
vtkPTableToStructuredGrid.cxx
vtkPYoungsMaterialInterface.cxx
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
vtkVPICReader.cxx
vtkWindBladeReader.cxx
vtkXMLPHierarchicalBoxDataWriter.cxx
vtkXMLPMultiBlockDataWriter.cxx
)

IF(VTK_USE_RENDERING)
  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkClientServerCompositePass.cxx
    vtkCompositeRenderManager.cxx
    vtkCompositeRGBAPass.cxx
    vtkCompositeZPass.cxx
    vtkImageRenderManager.cxx
    vtkMemoryLimitImageDataStreamer.cxx
    vtkParallelRenderManager.cxx
    vtkPImageWriter.cxx
    vtkPipelineSize.cxx
    vtkSynchronizedRenderers.cxx
    vtkSynchronizedRenderWindows.cxx
    vtkTreeCompositer.cxx
    )

# The set of source files to be encoded.
SET(shader_files
  vtkCompositeZPassShader_fs.glsl
  )
ENDIF(VTK_USE_RENDERING)

IF(VTK_HAS_EXODUS)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkExodusIIWriter.cxx
  )
ENDIF(VTK_HAS_EXODUS)

SET_SOURCE_FILES_PROPERTIES(
vtkCommunicator 
vtkMultiProcessController
vtkMultiProcessStream
vtkPStreamTracer
vtkProcess
ABSTRACT
)

SET_SOURCE_FILES_PROPERTIES(
vtkMultiProcessStream
vtkCompositeRGBAPass
vtkCompositeZPass
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

  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})

  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkMPICommunicator.cxx
    vtkMPIController.cxx 
    vtkPNetCDFPOPReader.cxx
    )
  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ELSE (VTK_USE_MPI)
  ADD_DEFINITIONS("-DUSE_SERIAL_COSMO")
ENDIF (VTK_USE_MPI)

IF (NOT VTK_LEGACY_REMOVE)
  SET(Kit_Deprecated_SRCS
    )
  IF (VTK_USE_MPI)
    SET(Kit_Deprecated_SRCS ${Kit_Deprecated_SRCS}
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
  SET(KIT_LIBS ${KIT_LIBS} ${Boost_LIBRARIES})
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
  IF (MPI_LIBRARY) 
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_LIBRARY}")   
  ELSE (MPI_LIBRARY)   
    MESSAGE("Could not find the required MPI libraries")
  ENDIF (MPI_LIBRARY)   
  IF (MPI_EXTRA_LIBRARY)   
    SET(KIT_LIBS ${KIT_LIBS} "${MPI_EXTRA_LIBRARY}")   
  ENDIF (MPI_EXTRA_LIBRARY) 
ENDIF (VTK_USE_MPI)

SET(Kit_EXTRA_SRCS
  ${VTK_BINARY_DIR}/Parallel/vtkSocketCommunicatorHash.h
  )
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

IF(VTK_USE_RENDERING)
# Create custom commands to encode each assembly file into a C string
# literal in a header file.

SET(shader_h_files)
FOREACH(file ${shader_files})
  GET_FILENAME_COMPONENT(file_we ${file} NAME_WE)
  SET(src ${VTK_SOURCE_DIR}/Parallel/${file})
  SET(res ${VTK_BINARY_DIR}/Parallel/${file_we}.cxx)
  SET(resh ${VTK_BINARY_DIR}/Parallel/${file_we}.h)
  SET(shader_h_files ${shader_h_files} "${resh}")
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file_we} --build-header VTK_PARALLEL_EXPORT vtkSystemIncludes.h
    )
  SET(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${res})
ENDFOREACH(file)
ENDIF(VTK_USE_RENDERING)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

# Generate "vtkSocketCommunicatorHash.h".
ADD_CUSTOM_COMMAND(
  OUTPUT ${VTK_BINARY_DIR}/Parallel/vtkSocketCommunicatorHash.h
  DEPENDS ${VTK_SOURCE_DIR}/Parallel/vtkSocketCommunicator.cxx
          vtkHashSource
  COMMAND vtkHashSource
    ${VTK_SOURCE_DIR}/Parallel/vtkSocketCommunicator.cxx
    vtkSocketCommunicatorHash vtkSocketCommunicatorHash.h
  )

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
