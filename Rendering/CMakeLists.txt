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
vtkHull
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
  IF (OPENGL_LIB_PATH)
    SOURCE_FILES(RenderingOpenGL_SRCS 
      vtkOpenGLImageWindow
      vtkOpenGLRenderWindow
      vtkXImageWindow
      vtkXOpenGLTextMapper
      vtkXRenderWindowInteractor
      vtkXRenderWindow
      vtkXTextMapper)
    LINK_LIBRARIES (${OPENGL_LIBRARY})
    LINK_DIRECTORIES (${OPENGL_LIB_PATH})
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_PATH})
    ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS)
  ELSE (OPENGL_LIB_PATH)
    ADD_LIBRARY(vtkRendering Rendering_SRCS)
  ENDIF (OPENGL_LIB_PATH)
ENDIF (WIN32)


IF (VTK_WRAP_TCL)
  SOURCE_FILES(RenderingTCL_SRCS 
    vtkTkRenderWidget vtkTkImageViewerWidget vtkTkImageWindowWidget)

  # add in the Tk values if found
  IF (TK_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
  ENDIF (TK_INCLUDE_PATH)
  IF (TK_LIBRARY)
    LINK_LIBRARIES (${TK_LIBRARY})
  ENDIF (TK_LIBRARY)

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
               RenderingTCL_SRCS 
               Rendering_SRCS 
               RenderingOpenGL_SRCS)
  ADD_LIBRARY(vtkRenderingTCL RenderingTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkRenderingTCL 
                         vtkGraphicsTCL 
                         vtkImagingTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL)
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
    TARGET_LINK_LIBRARIES (vtkCommonJava ${JAVA_AWT_LIBRARY})
  ENDIF (JAVA_AWT_LIBRARY)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkRendering
)


