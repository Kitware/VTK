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
vtkIVExporter
vtkImageActor
vtkImageMapper
vtkImager
vtkImagerCollection
vtkImageViewer
vtkImageViewer2
vtkImageWindow
vtkImagingFactory
vtkImporter
vtkInteractorStyle
vtkInteractorStyleFlight
vtkInteractorStyleImage
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
vtkLabeledDataMapper
vtkLight
vtkLightCollection
vtkLightKit 
vtkMapper
vtkMapperCollection
vtkOBJExporter 
vtkOOGLExporter
vtkParallelCoordinatesActor
vtkPicker
vtkPointPicker
vtkPolyDataMapper
vtkPolyDataMapper2D
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
vtkVRMLExporter
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


IF (VTK_USE_VOLUMEPRO)
  SOURCE_FILES ( Rendering_SRCS
    vtkVolumeProVG500Mapper
    vtkOpenGLVolumeProVG500Mapper
    )
ENDIF (VTK_USE_VOLUMEPRO)


SOURCE_FILES ( RenderingOpenGL_SRCS
               vtkOpenGLActor
               vtkOpenGLCamera
               vtkOpenGLImageActor
               vtkOpenGLImageMapper
               vtkOpenGLImager
               vtkOpenGLLight
               vtkOpenGLPolyDataMapper
               vtkOpenGLPolyDataMapper2D
               vtkOpenGLProperty
               vtkOpenGLRenderer
               vtkOpenGLRenderWindow
               vtkOpenGLTexture
               vtkOpenGLVolumeRayCastMapper
               vtkOpenGLVolumeTextureMapper2D
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
vtkOpenGLRenderWindow
vtkProp3D
vtkRayBounder
vtkVolumeMapper 
vtkVolumeRayCastFunction 
vtkVolumeReader 
vtkVolumeTextureMapper 
)


IF(UNIX)
  IF(NOT WIN32)
    IF(NOT APPLE)
      SOURCE_FILES ( Rendering_SRCS
                   vtkXImageWindow
                   vtkXRenderWindowInteractor
                   vtkXTextMapper)
    ENDIF(NOT APPLE)
  ENDIF(NOT WIN32)
ENDIF(UNIX)

IF (WIN32)
  SET(HAVE_OPENGL_SRCS ON)
  SOURCE_FILES(RenderingOpenGL_SRCS 
               vtkWin32OpenGLRenderWindow 
               vtkWin32RenderWindowInteractor
               vtkWin32OpenGLImageWindow
               vtkWin32OpenGLTextMapper
               vtkWin32TextMapper)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  IF(MESA_OS_LIBRARY)
    LINK_LIBRARIES( ${MESA_OS_LIBRARY} )
  ENDIF(MESA_OS_LIBRARY)
ELSE (WIN32)
  IF (APPLE)
    SET(HAVE_OPENGL_SRCS ON)
    SOURCE_FILES(RenderingOpenGL_SRCS 
		 vtkQuartzImageWindow
                 vtkQuartzTextMapper
                 vtkQuartzRenderWindowInteractor
                 vtkQuartzRenderWindow
                 vtkQuartzWindow
                 vtkQuartzGLView
                 vtkQuartzWindowController)
    WRAP_EXCLUDE_FILES(vtkQuartzGLView
		       vtkQuartzWindow
		       vtkQuartzWindowController)
    LINK_LIBRARIES (${OPENGL_LIBRARY} )
    IF(MESA_OS_LIBRARY)
      LINK_LIBRARIES( ${MESA_OS_LIBRARY} )
    ENDIF(MESA_OS_LIBRARY)

    ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS)
  ELSE (APPLE)
    IF (OPENGL_LIBRARY)
      SET(HAVE_OPENGL_SRCS ON)
      SOURCE_FILES(RenderingOpenGL_SRCS 
                   vtkOpenGLImageWindow
                   vtkXOpenGLRenderWindow
                   vtkXOpenGLTextMapper)
      ABSTRACT_FILES(
                     vtkXImageWindow
                     vtkXTextMapper)
      LINK_LIBRARIES (${OPENGL_LIBRARY} )
     IF(MESA_OS_LIBRARY)
       LINK_LIBRARIES( ${OSMESA_LIBRARY} )
     ENDIF(MESA_OS_LIBRARY)

    ENDIF (OPENGL_LIBRARY)
  ENDIF (APPLE)
ENDIF (WIN32)

IF (VTK_MANGLE_MESA)
  IF (MESA_LIBRARY)
    SET(HAVE_OPENGL_SRCS ON)
    SOURCE_FILES ( RenderingOpenGL_SRCS
                 vtkMesaActor
                 vtkMesaCamera
                 vtkMesaImageActor
                 vtkMesaImageMapper
                 vtkMesaImager
                 vtkMesaLight
                 vtkMesaPolyDataMapper
                 vtkMesaPolyDataMapper2D
                 vtkMesaProperty
                 vtkMesaRenderer
                 vtkMesaTexture
                 vtkMesaVolumeRayCastMapper
                 vtkMesaVolumeTextureMapper2D
                 vtkMesaImageWindow
                 vtkMesaRenderWindow
                 vtkXMesaRenderWindow
                 vtkXMesaTextMapper
                )

    LINK_LIBRARIES (${MESA_LIBRARY})
    IF(MESA_OS_LIBRARY)
      LINK_LIBRARIES( ${MESA_OS_LIBRARY} )
    ENDIF(MESA_OS_LIBRARY)
    IF (MESA_INCLUDE_PATH)
      INCLUDE_DIRECTORIES(${MESA_INCLUDE_PATH})
    ENDIF(MESA_INCLUDE_PATH)     
  ENDIF (MESA_LIBRARY)
ENDIF(VTK_MANGLE_MESA)

IF (HAVE_OPENGL_SRCS)
  ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS)
ELSE (HAVE_OPENGL_SRCS)
  ADD_LIBRARY(vtkRendering Rendering_SRCS)
ENDIF (HAVE_OPENGL_SRCS)

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
    # look for the internals header files in a few places
    IF (TK_LIBRARY)
      IF (TK_LIBRARY MATCHES tk83.lib)
        FIND_PATH(TK_INTERNAL_PATH tkWinInt.h 
          ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk83)
      ENDIF (TK_LIBRARY MATCHES tk83.lib)
      IF (TK_LIBRARY MATCHES tk82.lib)
        FIND_PATH(TK_INTERNAL_PATH tkWinInt.h 
          ${VTK_SOURCE_DIR}/Rendering/tkInternals)
      ENDIF (TK_LIBRARY MATCHES tk82.lib)
    ENDIF (TK_LIBRARY)
    IF (TK_INTERNAL_PATH)
      INCLUDE_DIRECTORIES(${TK_INTERNAL_PATH})
    ENDIF (TK_INTERNAL_PATH) 
    # for builds against the source tree on windows
    FIND_PATH(TK_XLIB_PATH X11/Xlib.h ${TK_INCLUDE_PATH} ${TK_INCLUDE_PATH}/../xlib)
    IF (TK_XLIB_PATH)
      INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
    ENDIF (TK_XLIB_PATH) 
  ELSE (WIN32)
    SOURCE_FILES(RenderingTCL_SRCS vtkXRenderWindowTclInteractor)
  ENDIF (WIN32)  

  IF (HAVE_OPENGL_SRCS)
    VTK_WRAP_TCL(vtkRenderingTCL
               SOURCES 
               RenderingTCL_SRCS 
               Rendering_SRCS 
               RenderingOpenGL_SRCS
               COMMANDS
               vtkTkRenderWidget
               vtkTkImageViewerWidget
               vtkTkImageWindowWidget)
  ELSE (HAVE_OPENGL_SRCS)
    VTK_WRAP_TCL(vtkRenderingTCL
               SOURCES 
               RenderingTCL_SRCS 
               Rendering_SRCS 
               COMMANDS
               vtkTkRenderWidget
               vtkTkImageViewerWidget
               vtkTkImageWindowWidget)
  ENDIF (HAVE_OPENGL_SRCS)
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

  INSTALL_TARGETS(/lib/vtk vtkRenderingTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  IF (HAVE_OPENGL_SRCS)
    VTK_WRAP_PYTHON(vtkRenderingPython 
                 RenderingPython_SRCS 
                 Rendering_SRCS
                 RenderingOpenGL_SRCS)
  ELSE (HAVE_OPENGL_SRCS)
    VTK_WRAP_PYTHON(vtkRenderingPython 
                 RenderingPython_SRCS 
                 Rendering_SRCS)
  ENDIF (HAVE_OPENGL_SRCS)
  
  IF (APPLE)  
    ADD_LIBRARY(vtkRenderingPython MODULE RenderingPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkRenderingPython SHARED RenderingPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkRenderingPython 
                           vtkGraphicsPython
                           vtkImagingPython
                           vtkFilteringPython
                           vtkCommonPython)
  ENDIF (APPLE)

  TARGET_LINK_LIBRARIES (vtkRenderingPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkRenderingPython)
  IF (TK_LIBRARY)
     SOURCE_FILES(RenderingPythonTkWidgets_SRCS 
        vtkTkWidgetsInit
        vtkTkRenderWidgetPython
        vtkTkImageViewerWidgetPython
        vtkTkImageWindowWidgetPython)
     ADD_LIBRARY(vtkRenderingPythonTkWidgets SHARED 
                 RenderingPythonTkWidgets_SRCS)
     TARGET_LINK_LIBRARIES (vtkRenderingPythonTkWidgets 
                            ${TCL_LIBRARY}
                            ${TK_LIBRARY})
     INSTALL_TARGETS(/lib/vtk vtkRenderingPythonTkWidgets)
  ENDIF (TK_LIBRARY)

ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkRenderingJava 
                RenderingJava_SRCS 
                Rendering_SRCS)
  ADD_LIBRARY(vtkRenderingJava SHARED RenderingJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkRenderingJava 
                         vtkCommonJava 
                         vtkFilteringJava
                         vtkImagingJava 
                         vtkGraphicsJava)
  IF (JAVA_AWT_LIBRARY)
    TARGET_LINK_LIBRARIES (vtkRenderingJava ${JAVA_AWT_LIBRARY})
  ENDIF (JAVA_AWT_LIBRARY)

  INSTALL_TARGETS(/lib/vtk vtkRenderingJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkRendering
)

IF(VTK_USE_X)
LINK_LIBRARIES(-lXt)
LINK_LIBRARIES(${CMAKE_X_LIBS})
ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(VTK_USE_X)

INSTALL_TARGETS(/lib/vtk vtkRendering)
INSTALL_FILES(/include/vtk .h Rendering_SRCS vtkRender)
IF (HAVE_OPENGL_SRCS)
  INSTALL_FILES(/include/vtk .h RenderingOpenGL_SRCS)
ENDIF (HAVE_OPENGL_SRCS))

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/Rendering/tkInternals/.NoDartCoverage 
${VTK_BINARY_DIR}/Rendering/tkInternals/.NoDartCoverage)
