SET( Rendering_SRCS
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
vtkInteractorEventRecorder.cxx
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

IF (VTK_HAVE_VG500)
  SET( Rendering_SRCS ${Rendering_SRCS}
    vtkVolumeProVG500Mapper.cxx
    vtkOpenGLVolumeProVG500Mapper.cxx
    )
ENDIF (VTK_HAVE_VG500)
IF (VTK_HAVE_VP1000)
  SET( Rendering_SRCS ${Rendering_SRCS}
    vtkVolumeProVP1000Mapper.cxx
    vtkOpenGLVolumeProVP1000Mapper.cxx
    )
ENDIF (VTK_HAVE_VP1000)

SET( RenderingOpenGL_SRCS
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLImageActor.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLTexture.cxx
  vtkOpenGLVolumeRayCastMapper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
vtkAbstractMapper3D
vtkAbstractPicker
vtkAbstractPropPicker
vtkCuller
vtkDirectionEncoder
vtkEncodedGradientEstimator
vtkExporter
vtkImporter
vtkInteractorObserver
vtkMapper
vtkOpenGLRenderWindow
vtkProp3D
vtkVolumeMapper
vtkVolumeRayCastFunction
vtkVolumeTextureMapper
ABSTRACT
)

# Freetype stuff

SET(Rendering_SRCS ${Rendering_SRCS}
  vtkFreeTypeFontCache.cxx
)
SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
  vtkOpenGLFreeTypeTextMapper.cxx
)
SET_SOURCE_FILES_PROPERTIES(
  vtkFreeTypeFontCache
  WRAP_EXCLUDE
)
SET(Rendering_FT_SRCS 
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

IF(VTK_USE_X)
  SET( Rendering_SRCS ${Rendering_SRCS}
    vtkXRenderWindowInteractor.cxx)
ENDIF(VTK_USE_X)

# Build a list of libraries to link to vtkRendering.
SET(Rendering_LIBS vtkGraphics vtkImaging)

IF(VTK_USE_VOLUMEPRO)
  IF(VTK_HAVE_VG500)
    SET(Rendering_LIBS ${Rendering_LIBS} ${VLI_LIBRARY_FOR_VG500})
  ENDIF(VTK_HAVE_VG500)
  IF(VTK_HAVE_VP1000)
    SET(Rendering_LIBS ${Rendering_LIBS} ${VLI_LIBRARY_FOR_VP1000})
  ENDIF(VTK_HAVE_VP1000)
ENDIF(VTK_USE_VOLUMEPRO)


IF (WIN32)
  IF(VTK_USE_X)
    IF (OPENGL_LIBRARY)
      SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                   vtkXOpenGLRenderWindow.cxx)
      SET (Rendering_LIBS ${Rendering_LIBS} ${OPENGL_LIBRARY} )
    ENDIF (OPENGL_LIBRARY)    
  ELSE(VTK_USE_X)
    SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                 vtkWin32OpenGLRenderWindow.cxx
                 vtkWin32RenderWindowInteractor.cxx)
    SET (Rendering_LIBS ${Rendering_LIBS} ${OPENGL_LIBRARY} )
  ENDIF(VTK_USE_X)
ELSE (WIN32)
  IF (APPLE)
    IF(VTK_USE_COCOA)
      SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                   vtkCocoaRenderWindowInteractor.mm
                   vtkCocoaRenderWindow.mm
                   vtkCocoaWindow.mm
                   vtkCocoaGLView.mm)
      SET_SOURCE_FILES_PROPERTIES(vtkCocoaGLView vtkCocoaWindow WRAP_EXCLUDE)
      SET (Rendering_LIBS ${Rendering_LIBS} ${OPENGL_LIBRARY} "-framework Cocoa")
    ENDIF(VTK_USE_COCOA)
    IF(VTK_USE_CARBON)
      SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                   vtkCarbonRenderWindowInteractor.cxx
                   vtkCarbonRenderWindow.cxx)
      SET (Rendering_LIBS ${Rendering_LIBS} ${OPENGL_LIBRARY} "-framework Carbon")
    ENDIF(VTK_USE_CARBON)
    IF(VTK_USE_X)
      IF (OPENGL_LIBRARY)
        SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                     vtkXOpenGLRenderWindow.cxx)
        SET (Rendering_LIBS ${Rendering_LIBS} ${OPENGL_LIBRARY} )
      ENDIF (OPENGL_LIBRARY)
    ENDIF(VTK_USE_X)
  ELSE (APPLE)
    IF (OPENGL_LIBRARY)
      SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                   vtkXOpenGLRenderWindow.cxx)
      SET (Rendering_LIBS ${Rendering_LIBS} ${OPENGL_LIBRARY} )
    ENDIF (OPENGL_LIBRARY)
  ENDIF (APPLE)
ENDIF (WIN32)

IF (VTK_USE_MANGLED_MESA)
  IF (MANGLED_MESA_LIBRARY)
    SET ( RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
                 vtkMesaActor.cxx
                 vtkMesaCamera.cxx
                 vtkMesaImageActor.cxx
                 vtkMesaImageMapper.cxx
                 vtkMesaLight.cxx
                 vtkMesaPolyDataMapper.cxx
                 vtkMesaPolyDataMapper2D.cxx
                 vtkMesaProperty.cxx
                 vtkMesaRenderWindow.cxx
                 vtkMesaRenderer.cxx
                 vtkMesaTexture.cxx
                 vtkMesaVolumeRayCastMapper.cxx
                 vtkMesaVolumeTextureMapper2D.cxx
                 vtkXMesaRenderWindow.cxx
                )
    SET(RenderingOpenGL_SRCS ${RenderingOpenGL_SRCS}
      vtkMesaFreeTypeTextMapper.cxx
    )
    SET_SOURCE_FILES_PROPERTIES(vtkMesaRenderWindow ABSTRACT)
    SET (Rendering_LIBS ${Rendering_LIBS} ${MANGLED_MESA_LIBRARY})
    IF(MANGLED_OSMESA_LIBRARY)
      SET( Rendering_LIBS ${Rendering_LIBS} ${MANGLED_OSMESA_LIBRARY} )
    ENDIF(MANGLED_OSMESA_LIBRARY)
  ENDIF (MANGLED_MESA_LIBRARY)
ELSE (VTK_USE_MANGLED_MESA)
  IF(VTK_OPENGL_HAS_OSMESA)
    IF (OSMESA_LIBRARY)
      SET(Rendering_LIBS ${Rendering_LIBS} ${OSMESA_LIBRARY})
    ENDIF (OSMESA_LIBRARY)
  ENDIF(VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_USE_MANGLED_MESA)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkRenderingInstantiator RenderingInstantiator_SRCS
  ${Rendering_SRCS} ${RenderingOpenGL_SRCS}
  EXPORT_MACRO VTK_RENDERING_EXPORT
  HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkRendering 
    ${Rendering_SRCS} 
    ${Rendering_FT_SRCS} 
    ${RenderingOpenGL_SRCS}
    ${RenderingInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkRendering PROPERTIES LINK_FLAGS "-init _oft_initRen")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

TARGET_LINK_LIBRARIES(vtkRendering ${Rendering_LIBS})

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Rendering/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Rendering/LocalUserOptions.cmake OPTIONAL)

TARGET_LINK_LIBRARIES(vtkRendering
  vtkftgl
  vtkfreetype
)

IF (VTK_WRAP_TCL)
  IF(NOT VTK_USE_COCOA)
    SET(RenderingTCL_SRCS
       vtkTkRenderWidget.cxx
       vtkTkImageViewerWidget.cxx)
  ENDIF(NOT VTK_USE_COCOA)

  # find the tk internals for win32
  IF (NOT WIN32)
    IF(NOT APPLE)
      SET(RenderingTCL_SRCS ${RenderingTCL_SRCS} 
        vtkXRenderWindowTclInteractor.cxx)
    ENDIF (NOT APPLE)
  ENDIF (NOT WIN32) 

  IF (NOT VTK_DISABLE_TK_INIT)
    VTK_WRAP_TCL2(vtkRenderingTCL
                 SOURCES
                 RenderingTCL_SRCS
                 ${Rendering_SRCS}
                 ${RenderingOpenGL_SRCS}
                 COMMANDS
                 vtkTkRenderWidget
                 vtkTkImageViewerWidget
                 )
  ELSE (NOT VTK_DISABLE_TK_INIT)
    VTK_WRAP_TCL2(vtkRenderingTCL
                 SOURCES
                 RenderingTCL_SRCS
                 ${Rendering_SRCS}
                 ${RenderingOpenGL_SRCS}
                 )
  ENDIF (NOT VTK_DISABLE_TK_INIT)

  IF (APPLE AND VTK_BUILD_SHARED_LIBS)
    ADD_LIBRARY(vtkRenderingTCL SHARED vtkOSXRenderingTclInit.c ${RenderingTCL_SRCS})
    IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
      SET_TARGET_PROPERTIES(vtkRenderingTCL PROPERTIES LINK_FLAGS "-init _oft_initRenOSXInit")
    ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  ELSE (APPLE AND VTK_BUILD_SHARED_LIBS)
    ADD_LIBRARY(vtkRenderingTCL ${RenderingTCL_SRCS})
  ENDIF (APPLE AND VTK_BUILD_SHARED_LIBS)

  TARGET_LINK_LIBRARIES (vtkRenderingTCL
                         vtkGraphicsTCL
                         vtkImagingTCL
                         vtkRendering
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
  VTK_WRAP_PYTHON2(vtkRenderingPython
                 RenderingPython_SRCS
                 ${Rendering_SRCS}
                 ${RenderingOpenGL_SRCS})
  ADD_LIBRARY(vtkRenderingPython MODULE ${RenderingPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkRenderingPython
                           vtkGraphicsPython
                           vtkImagingPython)
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkRenderingPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkRenderingPython vtkRendering)

  INSTALL_TARGETS(/lib/vtk vtkRenderingPython)
  IF (TK_LIBRARY)
     IF(NOT VTK_USE_COCOA)
       SET(RenderingPythonTkWidgets_SRCS
          vtkTkWidgetsInit.cxx
          vtkTkRenderWidgetPython.cxx
          vtkTkImageViewerWidgetPython.cxx
          )
         ADD_LIBRARY(vtkRenderingPythonTkWidgets SHARED
                     ${RenderingPythonTkWidgets_SRCS})
       TARGET_LINK_LIBRARIES (vtkRenderingPythonTkWidgets
                              vtkRendering
                              ${TCL_LIBRARY}
                              ${TK_LIBRARY})
       INSTALL_TARGETS(/lib/vtk vtkRenderingPythonTkWidgets)
     ENDIF(NOT VTK_USE_COCOA)
  ENDIF (TK_LIBRARY)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkRenderingJava
                RenderingJava_SRCS
                ${Rendering_SRCS})
  ADD_LIBRARY(vtkRenderingJava SHARED ${RenderingJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkRenderingJava
                         vtkImagingJava
                         vtkGraphicsJava
			 vtkRendering)
  IF (JAVA_AWT_LIBRARY)
    TARGET_LINK_LIBRARIES (vtkRenderingJava ${JAVA_AWT_LIBRARY})
  ENDIF (JAVA_AWT_LIBRARY)

  INSTALL_TARGETS(/lib/vtk vtkRenderingJava)
ENDIF (VTK_WRAP_JAVA)


IF(VTK_USE_X)
  TARGET_LINK_LIBRARIES(vtkRendering -lXt ${CMAKE_X_LIBS})
  ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(VTK_USE_X)

INSTALL_TARGETS(/lib/vtk vtkRendering)
INSTALL_FILES(/include/vtk .h ${Rendering_SRCS} ${RenderingOpenGL_SRCS})

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/Rendering/tkInternals/.NoDartCoverage
${VTK_BINARY_DIR}/Rendering/tkInternals/.NoDartCoverage)

IF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_INTERNAL_PATH)
  IF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk")
    SUBDIRS(tkInternals)
  ENDIF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk")
ENDIF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_INTERNAL_PATH)
