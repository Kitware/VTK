SET ( Parallel_SRCS
vtkBranchExtentTranslator.cxx
vtkCollectPolyData.cxx
vtkCommunicator.cxx
vtkCompositeManager.cxx
vtkCompositer.cxx
vtkCompressCompositer.cxx
vtkCutMaterial.cxx
vtkDummyController.cxx
vtkExtractPolyDataPiece.cxx
vtkExtractUnstructuredGridPiece.cxx
vtkInputPort.cxx
vtkMemoryLimitImageDataStreamer.cxx
vtkMultiProcessController.cxx
vtkOutputPort.cxx
vtkPassThroughFilter.cxx
vtkPCellDataToPointData.cxx
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
vtkPVGeometryFilter.cxx
vtkParallelFactory.cxx
vtkPieceScalars.cxx
vtkPipelineSize.cxx
vtkRTAnalyticSource.cxx
vtkRectilinearGridOutlineFilter.cxx
vtkSocketCommunicator.cxx
vtkSocketController.cxx
vtkTransmitPolyDataPiece.cxx
vtkTransmitUnstructuredGridPiece.cxx
vtkTreeComposite.cxx
vtkTreeCompositer.cxx
)


IF (CMAKE_USE_PTHREADS)
   SET(VTK_HAS_THREADS 1)
ENDIF (CMAKE_USE_PTHREADS)
IF (CMAKE_USE_SPROC)
   SET(VTK_HAS_THREADS 1)
ENDIF (CMAKE_USE_SPROC)
IF (CMAKE_USE_WIN32_THREADS)
   SET(VTK_HAS_THREADS 1)
ENDIF (CMAKE_USE_WIN32_THREADS)

IF (VTK_HAS_THREADS)
 SET ( Parallel_SRCS
      ${Parallel_SRCS}
      vtkSharedMemoryCommunicator.cxx
      vtkThreadedController.cxx)
ENDIF (VTK_HAS_THREADS)


SET_SOURCE_FILES_PROPERTIES(vtkCommunicator ABSTRACT)

IF (VTK_USE_MPI)
  INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
  SET ( Parallel_SRCS
    ${Parallel_SRCS}
    vtkMPICommunicator.cxx
    vtkMPIController.cxx 
    vtkMPIGroup.cxx)
ENDIF (VTK_USE_MPI)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkParallelInstantiator ParallelInstantiator_SRCS
                       ${Parallel_SRCS}
                       EXPORT_MACRO VTK_PARALLEL_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkParallel ${Parallel_SRCS} ${ParallelInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkParallel PROPERTIES LINK_FLAGS "-init _oft_initPar")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

IF (VTK_USE_MPI) 
  IF (MPI_LIBRARY) 
    TARGET_LINK_LIBRARIES (vtkParallel ${MPI_LIBRARY})   
  ELSE (MPI_LIBRARY)   
    MESSAGE("Could not find the required MPI libraries")
  ENDIF (MPI_LIBRARY)   
  IF (MPI_EXTRA_LIBRARY)   
    TARGET_LINK_LIBRARIES (vtkParallel ${MPI_EXTRA_LIBRARY})   
  ENDIF (MPI_EXTRA_LIBRARY) 
ENDIF (VTK_USE_MPI)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Parallel/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Parallel/LocalUserOptions.cmake OPTIONAL)

IF (WIN32)
  IF (NOT BORLAND)
    TARGET_LINK_LIBRARIES(vtkParallel wsock32)
  ENDIF (NOT BORLAND)
ENDIF (WIN32)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkParallelTCL ParallelTCL_SRCS ${Parallel_SRCS})
  ADD_LIBRARY(vtkParallelTCL ${ParallelTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkParallelTCL vtkParallel vtkIOTCL vtkRenderingTCL)
  INSTALL_TARGETS(/lib/vtk vtkParallelTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkParallelPython ParallelPython_SRCS ${Parallel_SRCS})
  ADD_LIBRARY(vtkParallelPython MODULE ${ParallelPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkParallelPython vtkIOPython vtkRenderingPython)
  ENDIF(NOT APPLE)
  TARGET_LINK_LIBRARIES (vtkParallelPython ${OPENGL_LIBRARY})

  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkParallelPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkParallelPython vtkParallel)

  INSTALL_TARGETS(/lib/vtk vtkParallelPython)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkParallelJava ParallelJava_SRCS ${Parallel_SRCS})
  ADD_LIBRARY(vtkParallelJava SHARED ${ParallelJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkParallelJava vtkParallel vtkRenderingJava vtkIOJava)
  INSTALL_TARGETS(/lib/vtk vtkParallelJava)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES (vtkParallel vtkRendering vtkIO)

INSTALL_TARGETS(/lib/vtk vtkParallel)
INSTALL_FILES(/include/vtk .h ${Parallel_SRCS})
IF (VTK_USE_MPI)
  INSTALL_FILES(/include/vtk "vtkMPI\\.h")
ENDIF (VTK_USE_MPI)
