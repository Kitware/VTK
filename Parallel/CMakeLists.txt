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
VTK_MAKE_INSTANTIATOR(vtkParallelInstantiator ParallelInstantiator_SRCS
                      ${Parallel_SRCS}
                      EXPORT_MACRO VTK_PARALLEL_EXPORT
                      HEADER_LOCATION ${VTK_BINARY_DIR})
ADD_LIBRARY(vtkParallel ${Parallel_SRCS} ${ParallelInstantiator_SRCS})

IF (VTK_USE_MPI) 
  IF (MPI_LIBRARY) 
    TARGET_LINK_LIBRARIES (vtkParallel ${MPI_LIBRARY})   
  ELSE (MPI_LIBRARY)   
    MESSAGE("Could not find the required MPI libraries")
  ENDIF (MPI_LIBRARY)   
  IF (MPI_EXTRA_LIBRARY)   
    TARGET_LINK_LIBRARIES (vtkParallel ${MPI_EXTRA_LIBRARY})   
  ENDIF (MPI_EXTRA_LIBRARY) 
  IF (MPI_INCLUDE_PATH)   
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF (MPI_INCLUDE_PATH)   
ENDIF (VTK_USE_MPI)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Parallel/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Parallel/LocalUserOptions.cmake OPTIONAL)

IF (WIN32)
  TARGET_LINK_LIBRARIES(vtkParallel wsock32)
ENDIF (WIN32)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkParallelTCL ParallelTCL_SRCS ${Parallel_SRCS})
  ADD_LIBRARY(vtkParallelTCL ${ParallelTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkParallelTCL vtkParallel vtkIOTCL vtkRenderingTCL)
  INSTALL_TARGETS(/lib/vtk vtkParallelTCL)

  IF (VTK_USE_MPI)

    # create the vtk executable
    CONFIGURE_FILE(
      ${VTK_SOURCE_DIR}/Wrapping/Tcl/tkAppInit.cxx.in 
      ${VTK_BINARY_DIR}/Parallel/paraTkAppInit.cxx IMMEDIATE)

    SET_SOURCE_FILES_PROPERTIES(${VTK_BINARY_DIR}/Parallel/paraTkAppInit.cxx 
                                COMPILE_FLAGS "-DVTK_COMPILED_USING_MPI")
    ADD_EXECUTABLE(pvtk ${VTK_BINARY_DIR}/Parallel/paraTkAppInit.cxx)

    TARGET_LINK_LIBRARIES (pvtk vtkParallelTCL vtkParallel)

    IF (VTK_USE_PATENTED)
      TARGET_LINK_LIBRARIES(pvtk vtkPatentedTCL vtkPatented)
    ENDIF (VTK_USE_PATENTED)
    IF (VTK_USE_HYBRID)
      TARGET_LINK_LIBRARIES(pvtk vtkHybridTCL vtkHybrid)
    ENDIF (VTK_USE_HYBRID)

    # add in opengl
    IF (WIN32)
      TARGET_LINK_LIBRARIES (pvtk  ${OPENGL_LIBRARY} )
    ELSE (WIN32)
      IF (OPENGL_LIBRARY)
        TARGET_LINK_LIBRARIES (pvtk ${OPENGL_LIBRARY})
      ENDIF (OPENGL_LIBRARY)
      IF(MESA_OS_LIBRARY)
        TARGET_LINK_LIBRARIES(pvtk ${MESA_OS_LIBRARY} )
      ENDIF(MESA_OS_LIBRARY)
    ENDIF (WIN32)

    # add in the Tk values if found
    IF (TK_INCLUDE_PATH)
      INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
    ENDIF (TK_INCLUDE_PATH)
    IF (TK_XLIB_PATH)
      INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
    ENDIF (TK_XLIB_PATH)
    IF (TK_LIBRARY)
      TARGET_LINK_LIBRARIES (pvtk ${TK_LIBRARY})
    ENDIF (TK_LIBRARY)

    # add in xwindows stuff
    IF (VTK_USE_X)
      TARGET_LINK_LIBRARIES(pvtk -lXt)
      TARGET_LINK_LIBRARIES(pvtk ${CMAKE_X_LIBS})
      ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
    ENDIF (VTK_USE_X)

    INSTALL_TARGETS(/bin pvtk)
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkParallelPython ParallelPython_SRCS ${Parallel_SRCS})
  IF (APPLE) 
    ADD_LIBRARY(vtkParallelPython MODULE ${ParallelPython_SRCS})
  ELSE (APPLE)
    ADD_LIBRARY(vtkParallelPython SHARED ${ParallelPython_SRCS})
    TARGET_LINK_LIBRARIES (vtkParallelPython vtkIOPython vtkRenderingPython)
  ENDIF (APPLE)
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
  VTK_WRAP_JAVA(vtkParallelJava ParallelJava_SRCS ${Parallel_SRCS})
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
