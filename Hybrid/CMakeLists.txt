SET(KIT Hybrid)
SET ( Kit_SRCS
vtk3DSImporter.cxx
vtk3DWidget.cxx
vtkArcPlotter.cxx
vtkBoxWidget.cxx
vtkCaptionActor2D.cxx
vtkCubeAxesActor2D.cxx
vtkDepthSortPolyData.cxx
vtkEarthSource.cxx
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
vtkPushImageFilterSample
vtkPushImageReader
vtkPushPipeline
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
    SET (Kit_SRCS ${Kit_SRCS} vtkWin32VideoSource.cxx)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)

  IF (VTK_USE_MATROX_IMAGING)
    SET (Kit_SRCS ${Kit_SRCS} vtkMILVideoSource.cxx)
    # Include directory needed for vtkMILVideoSource.cxx.
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(vtkVRML WRAP_EXCLUDE)

INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_HYBRID_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_initHyb")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtk${KIT}TCL KitTCL_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} vtkRenderingTCL vtkIOTCL)
  IF (WIN32)
    IF (VTK_USE_VIDEO_FOR_WINDOWS)
      TARGET_LINK_LIBRARIES (vtk${KIT}TCL vfw32)
    ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
    IF (VTK_USE_MATROX_IMAGING)
      TARGET_LINK_LIBRARIES (vtk${KIT}TCL ${MIL_LIBRARY})
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtkRenderingPython vtkIOPython)
  ENDIF(NOT APPLE)
  TARGET_LINK_LIBRARIES (vtk${KIT}Python ${OPENGL_LIBRARY})
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT})

  IF (WIN32)
    IF (VTK_USE_VIDEO_FOR_WINDOWS)
      TARGET_LINK_LIBRARIES (vtk${KIT}Python vfw32)
    ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
    IF (VTK_USE_MATROX_IMAGING)
      TARGET_LINK_LIBRARIES (vtk${KIT}Python ${MIL_LIBRARY})
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}Java vtk${KIT} vtkRenderingJava vtkIOJava)
  IF (WIN32)
    IF (VTK_USE_VIDEO_FOR_WINDOWS)
      TARGET_LINK_LIBRARIES (vtk${KIT}Java vfw32)
    ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
    IF (VTK_USE_MATROX_IMAGING)
      TARGET_LINK_LIBRARIES (vtk${KIT}Java ${MIL_LIBRARY})
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES (vtk${KIT} vtkRendering vtkIO)

IF (WIN32)
  IF (VTK_USE_VIDEO_FOR_WINDOWS)
    TARGET_LINK_LIBRARIES (vtk${KIT} vfw32)
  ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)
  IF (VTK_USE_MATROX_IMAGING)
    TARGET_LINK_LIBRARIES (vtk${KIT} ${MIL_LIBRARY})
  ENDIF (VTK_USE_MATROX_IMAGING)
ENDIF (WIN32)

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})
