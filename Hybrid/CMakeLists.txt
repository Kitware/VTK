SET ( Hybrid_SRCS
vtk3DSImporter.cxx
vtk3DWidget.cxx
vtkArcPlotter.cxx
vtkBoxWidget.cxx
vtkCaptionActor2D.cxx
vtkCubeAxesActor2D.cxx
vtkDepthSortPolyData.cxx
vtkEarthSource.cxx
vtkGreedyTerrainDecimation.cxx
vtkGridTransform.cxx
vtkImagePlaneWidget.cxx
vtkImageToPolyDataFilter.cxx
vtkImplicitModeller.cxx
vtkImplicitPlaneWidget.cxx
vtkIterativeClosestPointTransform.cxx
vtkLandmarkTransform.cxx
vtkLegendBoxActor.cxx
vtkLineWidget.cxx
vtkPCAAnalysisFilter.cxx
vtkPlaneWidget.cxx
vtkPointWidget.cxx
vtkPolyDataSourceWidget.cxx
vtkPolyDataToImageStencil.cxx
vtkProcrustesAlignmentFilter.cxx
vtkPushImageFilterSample.cxx
vtkPushImageReader.cxx
vtkPushPipeline.cxx
vtkRIBExporter.cxx
vtkRIBLight.cxx
vtkRIBProperty.cxx
vtkRenderLargeImage.cxx
vtkScalarBarWidget.cxx
vtkSphereWidget.cxx
vtkSplineWidget.cxx
vtkThinPlateSplineTransform.cxx
vtkTransformToGrid.cxx
vtkVRMLImporter.cxx
vtkVectorText.cxx
vtkVideoSource.cxx
vtkWeightedTransformFilter.cxx
vtkXYPlotActor.cxx
vtkXYPlotWidget.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtk3DWidget
vtkPolyDataSourceWidget
ABSTRACT
)

IF (WIN32)
  IF (VTK_USE_VIDEO_FOR_WINDOWS)
    SET (Hybrid_SRCS ${Hybrid_SRCS} vtkWin32VideoSource.cxx)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)

  IF (VTK_USE_MATROX_IMAGING)
    SET (Hybrid_SRCS ${Hybrid_SRCS} vtkMILVideoSource.cxx)
    # Include directory needed for vtkMILVideoSource.cxx.
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(vtkVRML WRAP_EXCLUDE)

INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkHybridInstantiator HybridInstantiator_SRCS
                       ${Hybrid_SRCS}
                       EXPORT_MACRO VTK_HYBRID_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkHybrid ${Hybrid_SRCS} ${HybridInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkHybrid PROPERTIES LINK_FLAGS "-init _oft_initHyb")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Hybrid/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Hybrid/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkHybridTCL HybridTCL_SRCS ${Hybrid_SRCS})
  ADD_LIBRARY(vtkHybridTCL ${HybridTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkHybridTCL vtkHybrid vtkRenderingTCL vtkIOTCL)
  IF (WIN32)
    IF (VTK_USE_VIDEO_FOR_WINDOWS)
      TARGET_LINK_LIBRARIES (vtkHybridTCL vfw32)
    ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
    IF (VTK_USE_MATROX_IMAGING)
      TARGET_LINK_LIBRARIES (vtkHybridTCL ${MIL_LIBRARY})
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  INSTALL_TARGETS(/lib/vtk vtkHybridTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkHybridPython HybridPython_SRCS ${Hybrid_SRCS})
  ADD_LIBRARY(vtkHybridPython MODULE ${HybridPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkHybridPython vtkRenderingPython vtkIOPython)
  ENDIF(NOT APPLE)
  TARGET_LINK_LIBRARIES (vtkHybridPython ${OPENGL_LIBRARY})
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkHybridPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkHybridPython vtkHybrid)

  IF (WIN32)
    IF (VTK_USE_VIDEO_FOR_WINDOWS)
      TARGET_LINK_LIBRARIES (vtkHybridPython vfw32)
    ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
    IF (VTK_USE_MATROX_IMAGING)
      TARGET_LINK_LIBRARIES (vtkHybridPython ${MIL_LIBRARY})
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  INSTALL_TARGETS(/lib/vtk vtkHybridPython)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkHybridJava HybridJava_SRCS ${Hybrid_SRCS})
  ADD_LIBRARY(vtkHybridJava SHARED ${HybridJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkHybridJava vtkHybrid vtkRenderingJava vtkIOJava)
  IF (WIN32)
    IF (VTK_USE_VIDEO_FOR_WINDOWS)
      TARGET_LINK_LIBRARIES (vtkHybridJava vfw32)
    ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
    IF (VTK_USE_MATROX_IMAGING)
      TARGET_LINK_LIBRARIES (vtkHybridJava ${MIL_LIBRARY})
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  INSTALL_TARGETS(/lib/vtk vtkHybridJava)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES (vtkHybrid vtkRendering vtkIO)

IF (WIN32)
  IF (VTK_USE_VIDEO_FOR_WINDOWS)
    TARGET_LINK_LIBRARIES (vtkHybrid vfw32)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
  IF (VTK_USE_MATROX_IMAGING)
    TARGET_LINK_LIBRARIES (vtkHybrid ${MIL_LIBRARY})
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

INSTALL_TARGETS(/lib/vtk vtkHybrid)
INSTALL_FILES(/include/vtk .h ${Hybrid_SRCS})
INSTALL_FILES(/include/vtk .h vtk3DS)
