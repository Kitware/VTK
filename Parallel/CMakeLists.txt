SET(KIT Parallel)
SET ( Kit_SRCS
vtkDuplicatePolyData.cxx
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
 SET ( Kit_SRCS
      ${Kit_SRCS}
      vtkSharedMemoryCommunicator.cxx
      vtkThreadedController.cxx)
ENDIF (VTK_HAS_THREADS)


SET_SOURCE_FILES_PROPERTIES(vtkCommunicator ABSTRACT)

IF (VTK_USE_MPI)
  INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
  SET ( Kit_SRCS
    ${Kit_SRCS}
    vtkMPICommunicator.cxx
    vtkMPIController.cxx 
    vtkMPIGroup.cxx)
ENDIF (VTK_USE_MPI)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_PARALLEL_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_initPar")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

IF (VTK_USE_MPI) 
  IF (MPI_LIBRARY) 
    TARGET_LINK_LIBRARIES (vtk${KIT} ${MPI_LIBRARY})   
  ELSE (MPI_LIBRARY)   
    MESSAGE("Could not find the required MPI libraries")
  ENDIF (MPI_LIBRARY)   
  IF (MPI_EXTRA_LIBRARY)   
    TARGET_LINK_LIBRARIES (vtk${KIT} ${MPI_EXTRA_LIBRARY})   
  ENDIF (MPI_EXTRA_LIBRARY) 
ENDIF (VTK_USE_MPI)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

IF (WIN32)
  IF (NOT BORLAND)
    TARGET_LINK_LIBRARIES(vtk${KIT} wsock32)
  ENDIF (NOT BORLAND)
ENDIF (WIN32)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtk${KIT}TCL KitTCL_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} vtkIOTCL vtkRenderingTCL)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtkIOPython vtkRenderingPython)
  ENDIF(NOT APPLE)
  TARGET_LINK_LIBRARIES (vtk${KIT}Python ${OPENGL_LIBRARY})

  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT})

  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}Java vtk${KIT} vtkRenderingJava vtkIOJava)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES (vtk${KIT} vtkRendering vtkIO)

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})
IF (VTK_USE_MPI)
  INSTALL_FILES(/include/vtk "vtkMPI\\.h")
ENDIF (VTK_USE_MPI)
