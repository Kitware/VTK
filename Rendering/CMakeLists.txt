SOURCE_FILES ( Rendering_SRCS
vtkAbstractMapper3D
vtkAbstractPicker
vtkAbstractPropPicker
vtkActor
vtkActorCollection
vtkAssembly
vtkAxisActor2D
vtkCamera
vtkCellPicker
vtkCuller
vtkCullerCollection
vtkDataSetMapper
vtkDirectionEncoder
vtkEncodedGradientEstimator
vtkEncodedGradientShader
vtkExporter
vtkFiniteDifferenceGradientEstimator
vtkFollower
vtkFrustumCoverageCuller
vtkGraphicsFactory
vtkImageActor
vtkImageMapper
vtkImager
vtkImagerCollection
vtkImageViewer
vtkImageWindow
vtkImagingFactory
vtkImporter
vtkInteractorStyle
vtkInteractorStyleFlight
vtkInteractorStyleJoystickActor
vtkInteractorStyleJoystickCamera
vtkInteractorStyleSwitch
vtkInteractorStyleTrackball
vtkInteractorStyleTrackballActor
vtkInteractorStyleTrackballCamera
vtkInteractorStyleUnicam
vtkInteractorStyleUser
vtkLODActor
vtkLODProp3D
vtkLight
vtkLightCollection
vtkLightKit 
vtkMapper
vtkMapperCollection
vtkPicker
vtkPointPicker
vtkPolyDataMapper
vtkPolyDataMapper2D
vtkProjectedPolyDataRayBounder
vtkProp3D
vtkProp3DCollection
vtkPropPicker
vtkProperty
vtkRayBounder
vtkRayCaster
vtkRecursiveSphereDirectionEncoder
vtkRenderWindow
vtkRenderWindowCollection
vtkRenderWindowInteractor
vtkRenderer
vtkRendererCollection
vtkRendererSource
vtkScalarBarActor
vtkScaledTextActor
vtkSelectVisiblePoints
vtkTextMapper
vtkTexture
vtkViewRays
vtkVolume
vtkVolumeCollection
vtkVolumeMapper
vtkVolumeProMapper 
vtkVolumeProperty
vtkVolumeRayCastCompositeFunction
vtkVolumeRayCastFunction
vtkVolumeRayCastIsosurfaceFunction
vtkVolumeRayCastMIPFunction
vtkVolumeRayCastMapper
vtkVolumeTextureMapper
vtkVolumeTextureMapper2D
vtkWorldPointPicker
)

ABSTRACT_FILES(
vtkAbstractMapper3D
vtkAbstractPicker 
vtkAbstractPropPicker 
vtkCuller
vtkDirectionEncoder
vtkEncodedGradientEstimator 
vtkExporter
vtkGraphicsFactory
vtkImporter
vtkMapper
vtkProp3D
vtkRayBounder
vtkVolumeMapper 
vtkVolumeRayCastFunction 
vtkVolumeReader 
vtkVolumeTextureMapper 
)

SOURCE_FILES ( RenderingOpenGL_SRCS
vtkOpenGLActor
vtkOpenGLCamera
vtkOpenGLImageActor
vtkOpenGLImageMapper
vtkOpenGLImager
vtkOpenGLLight
vtkOpenGLPolyDataMapper
vtkOpenGLPolyDataMapper2D
vtkOpenGLProjectedPolyDataRayBounder
vtkOpenGLProperty
vtkOpenGLRenderer
vtkOpenGLTexture
vtkOpenGLVolumeTextureMapper2D
)

IF (WIN32)
  SOURCE_FILES(RenderingOpenGL_SRCS 
     vtkWin32OpenGLRenderWindow 
     vtkWin32RenderWindowInteractor
     vtkWin32OpenGLImageWindow
     vtkWin32OpenGLTextMapper
     vtkWin32TextMapper)

  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS)
ELSE (WIN32)
  IF (OPENGL_LIBRARY)
    SOURCE_FILES(RenderingOpenGL_SRCS 
      vtkOpenGLImageWindow
      vtkOpenGLRenderWindow
      vtkXImageWindow
      vtkXOpenGLTextMapper
      vtkXRenderWindowInteractor
      vtkXRenderWindow
      vtkXTextMapper)
    LINK_LIBRARIES (${OPENGL_LIBRARY})
    ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS)
  ELSE (OPENGL_LIBRARY)
    ADD_LIBRARY(vtkRendering Rendering_SRCS)
  ENDIF (OPENGL_LIBRARY)
  IF (OPENGL_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_PATH})
  ENDIF(OPENGL_INCLUDE_PATH)     
ENDIF (WIN32)

# Allow the user to customize their build with some local options
#
INCLUDE (LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  SOURCE_FILES(RenderingTCL_SRCS 
     vtkTkRenderWidget 
     vtkTkImageViewerWidget 
     vtkTkImageWindowWidget)

  # add in the Tk values if found
  IF (TK_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
  ENDIF (TK_INCLUDE_PATH)

  # find the tk internals for win32
  IF (WIN32)
    FIND_PATH(TK_INTERNAL_PATH tkWinInt.h 
      ${VTK_SOURCE_DIR}/Rendering/tkInternals)
    IF (TK_INTERNAL_PATH)
      INCLUDE_DIRECTORIES(${TK_INTERNAL_PATH})
    ENDIF (TK_INTERNAL_PATH) 
  ELSE (WIN32)
    SOURCE_FILES(RenderingTCL_SRCS vtkXRenderWindowTclInteractor)
  ENDIF (WIN32)  

  VTK_WRAP_TCL(vtkRenderingTCL
               SOURCES 
               RenderingTCL_SRCS 
               Rendering_SRCS 
               RenderingOpenGL_SRCS
               COMMANDS
               vtkTkRenderWidget
               vtkTkImageViewerWidget
               vtkTkImageWindowWidget)
  ADD_LIBRARY(vtkRenderingTCL RenderingTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkRenderingTCL 
                         vtkGraphicsTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL
                         ${TCL_LIBRARY})
  IF (TK_LIBRARY)
    TARGET_LINK_LIBRARIES (vtkRenderingTCL ${TK_LIBRARY})
  ENDIF (TK_LIBRARY)
  INSTALL_TARGETS(/lib vtkRenderingTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkRenderingPython 
                 RenderingPython_SRCS 
                 Rendering_SRCS
                 RenderingOpenGL_SRCS)
  ADD_LIBRARY(vtkRenderingPython RenderingPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkRenderingPython 
                         vtkGraphicsPython
                         vtkImagingPython
                         vtkFilteringPython
                         vtkCommonPython)
  TARGET_LINK_LIBRARIES (vtkRenderingPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkRenderingPython)
  IF (TK_LIBRARY)
     SOURCE_FILES(RenderingPythonTkWidgets_SRCS 
        vtkTkRenderWidgetPython
        vtkTkImageViewerWidgetPython
        vtkTkImageWindowWidgetPython)
     ADD_LIBRARY(vtkRenderingPythonTkWidgets RenderingPythonTkWidgets_SRCS)
     TARGET_LINK_LIBRARIES (vtkRenderingPythonTkWidgets 
                            ${TCL_LIBRARY}
                            ${TK_LIBRARY})
     INSTALL_TARGETS(/lib vtkRenderingPythonTkWidgets)
  ENDIF (TK_LIBRARY)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkRenderingJava 
                RenderingJava_SRCS 
                Rendering_SRCS)
  ADD_LIBRARY(vtkRenderingJava RenderingJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkRenderingJava 
                         vtkCommonJava 
                         vtkFilteringJava
                         vtkImagingJava 
                         vtkGraphicsJava)
  IF (JAVA_AWT_LIBRARY)
    TARGET_LINK_LIBRARIES (vtkRenderingJava ${JAVA_AWT_LIBRARY})
  ENDIF (JAVA_AWT_LIBRARY)
  INSTALL_TARGETS(/lib vtkRenderingJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkRendering
)

IF(CMAKE_HAS_X)
LINK_LIBRARIES(-lXt)
LINK_LIBRARIES(${CMAKE_X_LIBS})
ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(CMAKE_HAS_X)



INSTALL_TARGETS(/lib vtkRendering)
INSTALL_FILES(/include/vtk .h Rendering_SRCS)
