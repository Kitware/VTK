SOURCE_FILES ( Rendering_SRCS
vtkAbstractMapper3D.cxx
vtkAbstractPicker.cxx
vtkAbstractPropPicker.cxx
vtkActor.cxx
vtkActorCollection.cxx
vtkAssembly.cxx
vtkAxisActor2D.cxx
vtkCamera.cxx
vtkCellPicker.cxx
vtkCuller.cxx
vtkCullerCollection.cxx
vtkDataSetMapper.cxx
vtkDirectionEncoder.cxx
vtkEncodedGradientEstimator.cxx
vtkEncodedGradientShader.cxx
vtkExporter.cxx
vtkFiniteDifferenceGradientEstimator.cxx
vtkFollower.cxx
vtkFrustumCoverageCuller.cxx
vtkGenericRenderWindowInteractor.cxx
vtkGraphicsFactory.cxx
vtkIVExporter.cxx
vtkImageActor.cxx
vtkImageMapper.cxx
vtkImageViewer.cxx
vtkImageViewer2.cxx
vtkImagingFactory.cxx
vtkImporter.cxx
vtkInteractorObserver.cxx
vtkInteractorStyle.cxx
vtkInteractorStyleFlight.cxx
vtkInteractorStyleImage.cxx
vtkInteractorStyleJoystickActor.cxx
vtkInteractorStyleJoystickCamera.cxx
vtkInteractorStyleSwitch.cxx
vtkInteractorStyleTerrain.cxx
vtkInteractorStyleTrackball.cxx
vtkInteractorStyleTrackballActor.cxx
vtkInteractorStyleTrackballCamera.cxx
vtkInteractorStyleUnicam.cxx
vtkInteractorStyleUser.cxx
vtkLODActor.cxx
vtkLODProp3D.cxx
vtkLabeledDataMapper.cxx
vtkLight.cxx
vtkLightCollection.cxx
vtkLightKit.cxx
vtkMapper.cxx
vtkMapperCollection.cxx
vtkOBJExporter.cxx
vtkOOGLExporter.cxx
vtkParallelCoordinatesActor.cxx
vtkPicker.cxx
vtkPointPicker.cxx
vtkPolyDataMapper.cxx
vtkPolyDataMapper2D.cxx
vtkProp3D.cxx
vtkProp3DCollection.cxx
vtkPropPicker.cxx
vtkProperty.cxx
vtkRecursiveSphereDirectionEncoder.cxx
vtkRenderWindow.cxx
vtkRenderWindowCollection.cxx
vtkRenderWindowInteractor.cxx
vtkRenderer.cxx
vtkRendererCollection.cxx
vtkRendererSource.cxx
vtkScalarBarActor.cxx
vtkScaledTextActor.cxx
vtkSelectVisiblePoints.cxx
vtkTextActor.cxx
vtkTextMapper.cxx
vtkTextProperty.cxx
vtkTexture.cxx
vtkVRMLExporter.cxx
vtkVolume.cxx
vtkVolumeCollection.cxx
vtkVolumeMapper.cxx
vtkVolumeProMapper.cxx
vtkVolumeProperty.cxx
vtkVolumeRayCastCompositeFunction.cxx
vtkVolumeRayCastFunction.cxx
vtkVolumeRayCastIsosurfaceFunction.cxx
vtkVolumeRayCastMIPFunction.cxx
vtkVolumeRayCastMapper.cxx
vtkVolumeTextureMapper.cxx
vtkVolumeTextureMapper2D.cxx
vtkWindowToImageFilter.cxx
vtkWorldPointPicker.cxx
)

IF (VTK_USE_FREETYPE)
  INCLUDE_DIRECTORIES(
    ${VTKFREETYPE_SOURCE_DIR}/include
    ${VTKFREETYPE_BINARY_DIR}
    ${VTKFTGL_SOURCE_DIR}/src
    ${VTKFTGL_BINARY_DIR}
  )
  SOURCE_FILES(Rendering_SRCS
    vtkOpenGLFreeTypeTextMapper.cxx
    fonts/face_arial.cxx
    fonts/face_arial_bold.cxx
    fonts/face_arial_bold_italic.cxx
    fonts/face_arial_italic.cxx
    fonts/face_courier.cxx
    fonts/face_courier_bold.cxx
    fonts/face_courier_bold_italic.cxx
    fonts/face_courier_italic.cxx
    fonts/face_times.cxx
    fonts/face_times_bold.cxx
    fonts/face_times_bold_italic.cxx
    fonts/face_times_italic.cxx
  )
  WRAP_EXCLUDE_FILES(
    face_arial
    face_arial_bold
    face_arial_bold_italic
    face_arial_italic
    face_courier
    face_courier_bold
    face_courier_bold_italic
    face_courier_italic
    face_times
    face_times_bold
    face_times_bold_italic
    face_times_italic
  )
  LINK_LIBRARIES(
    vtkfreetype
    vtkftgl
  )
ENDIF (VTK_USE_FREETYPE)

IF (VTK_HAVE_VG500)
  SOURCE_FILES ( Rendering_SRCS
    vtkVolumeProVG500Mapper.cxx
    vtkOpenGLVolumeProVG500Mapper.cxx
    )
ENDIF (VTK_HAVE_VG500)
IF (VTK_HAVE_VP1000)
  SOURCE_FILES ( Rendering_SRCS
    vtkVolumeProVP1000Mapper.cxx
    vtkOpenGLVolumeProVP1000Mapper.cxx
    )
ENDIF (VTK_HAVE_VP1000)


SOURCE_FILES ( RenderingOpenGL_SRCS
               vtkOpenGLActor.cxx
               vtkOpenGLCamera.cxx
               vtkOpenGLImageActor.cxx
               vtkOpenGLImageMapper.cxx
               vtkOpenGLLight.cxx
               vtkOpenGLPolyDataMapper.cxx
               vtkOpenGLPolyDataMapper2D.cxx
               vtkOpenGLProperty.cxx
               vtkOpenGLRenderer.cxx
               vtkOpenGLRenderWindow.cxx
               vtkOpenGLTexture.cxx
               vtkOpenGLVolumeRayCastMapper.cxx
               vtkOpenGLVolumeTextureMapper2D.cxx
              )

ABSTRACT_FILES(
vtkAbstractMapper3D
vtkAbstractPicker
vtkAbstractPropPicker
vtkCuller
vtkDirectionEncoder
vtkEncodedGradientEstimator
vtkExporter
vtkInteractorObserver
vtkImporter
vtkMapper
vtkOpenGLRenderWindow
vtkProp3D
vtkVolumeMapper
vtkVolumeRayCastFunction
vtkVolumeTextureMapper
)


IF(VTK_USE_X)
  SOURCE_FILES ( Rendering_SRCS
               vtkXRenderWindowInteractor.cxx
               vtkXTextMapper.cxx)
ENDIF(VTK_USE_X)

IF (WIN32)
  SET(HAVE_OPENGL_SRCS ON)
  SOURCE_FILES(RenderingOpenGL_SRCS
               vtkWin32OpenGLRenderWindow.cxx
               vtkWin32RenderWindowInteractor.cxx
               vtkWin32OpenGLTextMapper.cxx
               vtkWin32TextMapper.cxx)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
ELSE (WIN32)
  IF (APPLE)
    IF(VTK_USE_COCOA)
      SET(HAVE_OPENGL_SRCS ON)
      SOURCE_FILES(RenderingOpenGL_SRCS
                   vtkCocoaTextMapper.cxx
                   vtkCocoaRenderWindowInteractor.mm
                   vtkCocoaRenderWindow.mm
                   vtkCocoaWindow.mm
                   vtkCocoaGLView.mm)
      WRAP_EXCLUDE_FILES(vtkCocoaGLView
                         vtkCocoaWindow)
      LINK_LIBRARIES (${OPENGL_LIBRARY} )
    ENDIF(VTK_USE_COCOA)
    IF(VTK_USE_CARBON)
      SET(HAVE_OPENGL_SRCS ON)
      SOURCE_FILES(RenderingOpenGL_SRCS
                   vtkCarbonTextMapper.cxx
                   vtkCarbonRenderWindowInteractor.cxx
                   vtkCarbonRenderWindow.cxx)
      LINK_LIBRARIES (${OPENGL_LIBRARY} )
    ENDIF(VTK_USE_CARBON)
    IF(VTK_USE_X)
      IF (OPENGL_LIBRARY)
        SET(HAVE_OPENGL_SRCS ON)
        SOURCE_FILES(RenderingOpenGL_SRCS
                     vtkXOpenGLRenderWindow.cxx
                     vtkXOpenGLTextMapper.cxx)
        ABSTRACT_FILES(
                       vtkXTextMapper)
        LINK_LIBRARIES (${OPENGL_LIBRARY} )
      ENDIF (OPENGL_LIBRARY)
    ENDIF(VTK_USE_X)
  ELSE (APPLE)
    IF (OPENGL_LIBRARY)
      SET(HAVE_OPENGL_SRCS ON)
      SOURCE_FILES(RenderingOpenGL_SRCS
                   vtkXOpenGLRenderWindow.cxx
                   vtkXOpenGLTextMapper.cxx)
      ABSTRACT_FILES(
                     vtkXTextMapper)
      LINK_LIBRARIES (${OPENGL_LIBRARY} )
    ENDIF (OPENGL_LIBRARY)
  ENDIF (APPLE)
ENDIF (WIN32)

IF (VTK_MANGLE_MESA)
  IF (MESA_LIBRARY)
    SET(HAVE_OPENGL_SRCS ON)
    SOURCE_FILES ( RenderingOpenGL_SRCS
                 vtkMesaActor.cxx
                 vtkMesaCamera.cxx
                 vtkMesaImageActor.cxx
                 vtkMesaImageMapper.cxx
                 vtkMesaLight.cxx
                 vtkMesaPolyDataMapper.cxx
                 vtkMesaPolyDataMapper2D.cxx
                 vtkMesaProperty.cxx
                 vtkMesaRenderer.cxx
                 vtkMesaTexture.cxx
                 vtkMesaVolumeRayCastMapper.cxx
                 vtkMesaVolumeTextureMapper2D.cxx
                 vtkMesaRenderWindow.cxx
                 vtkXMesaRenderWindow.cxx
                 vtkXMesaTextMapper.cxx
                )
    ABSTRACT_FILES(vtkMesaRenderWindow)
    LINK_LIBRARIES (${MESA_LIBRARY})
    IF(MESA_OS_LIBRARY)
      LINK_LIBRARIES( ${MESA_OS_LIBRARY} )
    ENDIF(MESA_OS_LIBRARY)
    IF (MESA_INCLUDE_PATH)
      INCLUDE_DIRECTORIES(${MESA_INCLUDE_PATH})
    ENDIF(MESA_INCLUDE_PATH)    
  ENDIF (MESA_LIBRARY)
ELSE (VTK_MANGLE_MESA)
  IF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
    LINK_LIBRARIES(${MESA_OS_LIBRARY})
  ENDIF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_MANGLE_MESA)

IF(VTK_MAKE_INSTANTIATOR)
  IF(HAVE_OPENGL_SRCS)
    # Setup vtkInstantiator registration for this library's classes.
    VTK_MAKE_INSTANTIATOR(vtkRenderingInstantiator RenderingInstantiator_SRCS
                          Rendering_SRCS RenderingOpenGL_SRCS
                          EXPORT_MACRO VTK_RENDERING_EXPORT
                          HEADER_LOCATION ${VTK_BINARY_DIR})
    ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS
                             RenderingInstantiator_SRCS)
  ELSE(HAVE_OPENGL_SRCS)
    # Setup vtkInstantiator registration for this library's classes.
    VTK_MAKE_INSTANTIATOR(vtkRenderingInstantiator RenderingInstantiator_SRCS
                          Rendering_SRCS
                          EXPORT_MACRO VTK_RENDERING_EXPORT
                          HEADER_LOCATION ${VTK_BINARY_DIR})
    ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingInstantiator_SRCS)
  ENDIF(HAVE_OPENGL_SRCS)
  INSTALL_FILES(/include/vtk .h ${VTK_BINARY_DIR}/vtkRenderingInstantiator)
ELSE(VTK_MAKE_INSTANTIATOR)
  IF (HAVE_OPENGL_SRCS)
    ADD_LIBRARY(vtkRendering Rendering_SRCS RenderingOpenGL_SRCS)
  ELSE (HAVE_OPENGL_SRCS)
    ADD_LIBRARY(vtkRendering Rendering_SRCS)
  ENDIF (HAVE_OPENGL_SRCS)
ENDIF(VTK_MAKE_INSTANTIATOR)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Rendering/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Rendering/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  SOURCE_FILES(RenderingTCL_SRCS
     vtkTkRenderWidget.cxx
     vtkTkImageViewerWidget.cxx)

  IF (VTK_USE_X)
    SET(VTK_DISABLE_TK_INIT 0 CACHE BOOL "Allows the user to disable the automatic initialization of Tk widgets when loading the rendering library.")
    MARK_AS_ADVANCED(VTK_DISABLE_TK_INIT)
  ENDIF (VTK_USE_X)

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
    MARK_AS_ADVANCED(TK_XLIB_PATH)
    IF (TK_XLIB_PATH)
      INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
    ENDIF (TK_XLIB_PATH)
  ELSE (WIN32)
    IF (APPLE)
      INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering/tkOSXInternals)
    ELSE (APPLE)
      SOURCE_FILES(RenderingTCL_SRCS vtkXRenderWindowTclInteractor.cxx)
    ENDIF (APPLE)
  ENDIF (WIN32) 

  IF (HAVE_OPENGL_SRCS)
    IF (NOT VTK_DISABLE_TK_INIT)
      VTK_WRAP_TCL(vtkRenderingTCL
                 SOURCES
                 RenderingTCL_SRCS
                 Rendering_SRCS
                 RenderingOpenGL_SRCS
                 COMMANDS
                 vtkTkRenderWidget
                 vtkTkImageViewerWidget
                 )
    ELSE (NOT VTK_DISABLE_TK_INIT)
      VTK_WRAP_TCL(vtkRenderingTCL
                 SOURCES
                 RenderingTCL_SRCS
                 Rendering_SRCS
                 RenderingOpenGL_SRCS
                 )
    ENDIF (NOT VTK_DISABLE_TK_INIT)
  ELSE (HAVE_OPENGL_SRCS)
    IF (NOT VTK_DISABLE_TK_INIT)
      VTK_WRAP_TCL(vtkRenderingTCL
                 SOURCES
                 RenderingTCL_SRCS
                 Rendering_SRCS
                 COMMANDS
                 vtkTkRenderWidget
                 vtkTkImageViewerWidget
                 )
    ELSE (NOT VTK_DISABLE_TK_INIT)
      VTK_WRAP_TCL(vtkRenderingTCL
                 SOURCES
                 RenderingTCL_SRCS
                 Rendering_SRCS
                 )
    ENDIF (NOT VTK_DISABLE_TK_INIT)
  ENDIF (HAVE_OPENGL_SRCS)

  IF (APPLE)
    ADD_LIBRARY(vtkRenderingTCL SHARED RenderingTCL_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkRenderingTCL RenderingTCL_SRCS)
  ENDIF (APPLE)
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

  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkRenderingPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkRenderingPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)

  INSTALL_TARGETS(/lib/vtk vtkRenderingPython)
  IF (TK_LIBRARY)
     SOURCE_FILES(RenderingPythonTkWidgets_SRCS
        vtkTkWidgetsInit.cxx
        vtkTkRenderWidgetPython.cxx
        vtkTkImageViewerWidgetPython.cxx
        )
     ADD_LIBRARY(vtkRenderingPythonTkWidgets SHARED
                 RenderingPythonTkWidgets_SRCS)
     TARGET_LINK_LIBRARIES (vtkRenderingPythonTkWidgets
                            ${TCL_LIBRARY}
                            ${TK_LIBRARY})
     INSTALL_TARGETS(/lib/vtk vtkRenderingPythonTkWidgets)

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
       MARK_AS_ADVANCED(TK_XLIB_PATH)
       IF (TK_XLIB_PATH)
         INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
       ENDIF (TK_XLIB_PATH)
     ENDIF (WIN32) 

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
ENDIF (HAVE_OPENGL_SRCS)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/Rendering/tkInternals/.NoDartCoverage
${VTK_BINARY_DIR}/Rendering/tkInternals/.NoDartCoverage)
