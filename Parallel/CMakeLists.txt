SOURCE_FILES ( Parallel_SRCS
vtkBranchExtentTranslator 
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
vtkSocketCommunicator
vtkSocketController 
)

ABSTRACT_FILES( vtkCommunicator )

IF (VTK_USE_MPI)
  INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
  IF (MPI_LIBRARY)
    SOURCE_FILES ( Parallel_SRCS
      vtkMPICommunicator 
      vtkMPIController 
      vtkMPIGroup)
  LINK_LIBRARIES (${MPI_LIBRARY})
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF (MPI_LIBRARY)
ENDIF (VTK_USE_MPI)

ADD_LIBRARY(vtkParallel Parallel_SRCS)

IF (WIN32)
  LINK_LIBRARIES( wsock32 )
ENDIF (WIN32)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkParallelTCL ParallelTCL_SRCS Parallel_SRCS)
  ADD_LIBRARY(vtkParallelTCL ParallelTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkParallelTCL 
                         vtkIOTCL 
                         vtkRenderingTCL
                         vtkGraphicsTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkParallelTCL)

  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(pvtk tkAppInit)

    IF (HAVE_PNG)
      TARGET_LINK_LIBRARIES(pvtk ${PNG_LIBRARY})
    ENDIF (HAVE_PNG)

    TARGET_LINK_LIBRARIES (pvtk
			 vtkParallelTCL
                         vtkRenderingTCL
                         vtkIOTCL 
                         vtkGraphicsTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL
			 ${TCL_LIBRARY}
			 ${TK_LIBRARY})

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
    ENDIF (WIN32)

    # add in the Tk values if found
    IF (TK_INCLUDE_PATH)
      INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
    ENDIF (TK_INCLUDE_PATH)
    IF (TK_LIBRARY)
      TARGET_LINK_LIBRARIES (pvtk ${TK_LIBRARY})
    ENDIF (TK_LIBRARY)

    # add in xwindows stuff
    IF (CMAKE_HAS_X)
      TARGET_LINK_LIBRARIES(pvtk -lXt)
      TARGET_LINK_LIBRARIES(pvtk ${CMAKE_X_LIBS})
      ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
    ENDIF (CMAKE_HAS_X)

    INSTALL_TARGETS(/bin pvtk)
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  ADD_LIBRARY(vtkParallelPython ParallelPython_SRCS)
  VTK_WRAP_PYTHON(vtkParallelPython ParallelPython_SRCS Parallel_SRCS)
  TARGET_LINK_LIBRARIES (vtkParallelPython 
                         vtkIOPython
                         vtkRenderingPython
                         vtkGraphicsPython
                         vtkImagingPython
                         vtkFilteringPython
                         vtkCommonPython)
  TARGET_LINK_LIBRARIES (vtkParallelPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkParallelPython)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkParallelJava ParallelJava_SRCS Parallel_SRCS)
  ADD_LIBRARY(vtkParallelJava ParallelJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkParallelJava 
                         vtkCommonJava 
                         vtkFilteringJava
                         vtkImagingJava 
                         vtkGraphicsJava 
                         vtkRenderingJava
                         vtkIOJava)
  INSTALL_TARGETS(/lib vtkParallelJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkParallel
vtkRendering
vtkImaging
vtkGraphics
vtkIO
vtkFiltering 
vtkCommon 
)


INSTALL_TARGETS(/lib vtkParallel)
INSTALL_FILES(/include/vtk .h Parallel_SRCS)
