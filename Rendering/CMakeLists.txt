SET(KIT Rendering)
SET(UKIT RENDERING)
SET(KIT_TCL_LIBS vtkGraphicsTCL vtkImagingTCL ${TCL_LIBRARY})
IF (TK_LIBRARY)
  SET(KIT_TCL_LIBS ${KIT_TCL_LIBS} ${TK_LIBRARY})
ENDIF (TK_LIBRARY)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkGraphicsPythonD vtkImagingPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkGraphicsPython vtkImagingPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkGraphicsJava vtkImagingJava)
IF (JAVA_AWT_LIBRARY)
  SET(KIT_JAVA_LIBS ${KIT_JAVA_LIBS} ${JAVA_AWT_LIBRARY})
ENDIF (JAVA_AWT_LIBRARY)
SET(KIT_LIBS vtkGraphics vtkImaging vtkIO
  vtkftgl
  vtkfreetype
)

SET( Kit_SRCS
vtkAbstractMapper3D.cxx
vtkAbstractVolumeMapper.cxx
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
vtkInteractorStyleRubberBandZoom.cxx
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
vtkRayCastImageDisplayHelper.cxx
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
vtkTesting.cxx
vtkTextActor.cxx
vtkTextMapper.cxx
vtkTextProperty.cxx
vtkTexture.cxx
vtkUnstructuredGridBunykRayCastFunction.cxx
vtkUnstructuredGridVolumeMapper.cxx
vtkUnstructuredGridVolumeRayCastFunction.cxx
vtkUnstructuredGridVolumeRayCastMapper.cxx
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
  SET( Kit_SRCS ${Kit_SRCS}
    vtkVolumeProVG500Mapper.cxx
    vtkOpenGLVolumeProVG500Mapper.cxx
    )
ENDIF (VTK_HAVE_VG500)
IF (VTK_HAVE_VP1000)
  SET( Kit_SRCS ${Kit_SRCS}
    vtkVolumeProVP1000Mapper.cxx
    vtkOpenGLVolumeProVP1000Mapper.cxx
    )
ENDIF (VTK_HAVE_VP1000)

IF(VTK_USE_GL2PS)
  SET(Kit_SRCS ${Kit_SRCS} vtkGL2PSExporter.cxx)
  SET(Kit_GL2PS_SRCS ${VTK_SOURCE_DIR}/Utilities/gl2ps/gl2ps.c)
  ADD_DEFINITIONS(-DGL2PS_HAVE_ZLIB)
ENDIF(VTK_USE_GL2PS)

SET( KitOpenGL_SRCS
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLImageActor.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLTexture.cxx
  vtkOpenGLVolumeRayCastMapper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
vtkAbstractMapper3D
vtkAbstractVolumeMapper
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
vtkRayCastImageDisplayHelper
vtkUnstructuredGridVolumeMapper
vtkUnstructuredGridVolumeRayCastFunction
vtkVolumeMapper
vtkVolumeRayCastFunction
vtkVolumeTextureMapper
ABSTRACT
)

# Freetype stuff

SET(Kit_SRCS ${Kit_SRCS}
  vtkFreeTypeFontCache.cxx
)
SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
  vtkOpenGLFreeTypeTextMapper.cxx
)
SET_SOURCE_FILES_PROPERTIES(
  vtkFreeTypeFontCache
  WRAP_EXCLUDE
)
SET(Kit_FT_SRCS 
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
  SET( Kit_SRCS ${Kit_SRCS}
    vtkXRenderWindowInteractor.cxx)
ENDIF(VTK_USE_X)

# Build a list of libraries to link to vtkRendering.

IF(VTK_USE_VOLUMEPRO)
  IF(VTK_HAVE_VG500)
    SET(KIT_LIBS ${KIT_LIBS} ${VLI_LIBRARY_FOR_VG500})
  ENDIF(VTK_HAVE_VG500)
  IF(VTK_HAVE_VP1000)
    SET(KIT_LIBS ${KIT_LIBS} ${VLI_LIBRARY_FOR_VP1000})
  ENDIF(VTK_HAVE_VP1000)
ENDIF(VTK_USE_VOLUMEPRO)

IF(VTK_USE_GL2PS)
  SET(KIT_LIBS ${KIT_LIBS} ${VTK_ZLIB_LIBRARIES})
ENDIF(VTK_USE_GL2PS)

IF (WIN32)
  IF(VTK_USE_X)
    IF (OPENGL_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkXOpenGLRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_LIBRARY} )
    ENDIF (OPENGL_LIBRARY)    
  ELSE(VTK_USE_X)
    SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                 vtkWin32OpenGLRenderWindow.cxx
                 vtkWin32RenderWindowInteractor.cxx)
    SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_LIBRARY} )
  ENDIF(VTK_USE_X)
ELSE (WIN32)
  IF (APPLE)
    IF(VTK_USE_COCOA)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkCocoaRenderWindowInteractor.mm
                   vtkCocoaRenderWindow.mm
                   vtkCocoaWindow.mm
                   vtkCocoaGLView.mm)
      SET_SOURCE_FILES_PROPERTIES(vtkCocoaGLView vtkCocoaWindow WRAP_EXCLUDE)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_LIBRARY} "-framework Cocoa")
    ENDIF(VTK_USE_COCOA)
    IF(VTK_USE_CARBON)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkCarbonRenderWindowInteractor.cxx
                   vtkCarbonRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_LIBRARY} "-framework Carbon")
    ENDIF(VTK_USE_CARBON)
    IF(VTK_USE_X)
      IF (OPENGL_LIBRARY)
        SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                     vtkXOpenGLRenderWindow.cxx)
        SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_LIBRARY})
      ENDIF (OPENGL_LIBRARY)
    ENDIF(VTK_USE_X)
  ELSE (APPLE)
    IF (OPENGL_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkXOpenGLRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_LIBRARY} )
    ENDIF (OPENGL_LIBRARY)
  ENDIF (APPLE)
ENDIF (WIN32)

IF (VTK_USE_MANGLED_MESA)
  IF (MANGLED_MESA_LIBRARY)
    SET ( KitOpenGL_SRCS ${KitOpenGL_SRCS}
                 vtkMesaActor.cxx
                 vtkMesaCamera.cxx
                 vtkMesaImageActor.cxx
                 vtkMesaImageMapper.cxx
                 vtkMesaLight.cxx
                 vtkMesaPolyDataMapper.cxx
                 vtkMesaPolyDataMapper2D.cxx
                 vtkMesaProperty.cxx
                 vtkMesaRayCastImageDisplayHelper.cxx
                 vtkMesaRenderWindow.cxx
                 vtkMesaRenderer.cxx
                 vtkMesaTexture.cxx
                 vtkMesaVolumeRayCastMapper.cxx
                 vtkMesaVolumeTextureMapper2D.cxx
                 vtkXMesaRenderWindow.cxx
                )
    SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
      vtkMesaFreeTypeTextMapper.cxx
    )
    SET_SOURCE_FILES_PROPERTIES(vtkMesaRenderWindow ABSTRACT)
    SET (KIT_LIBS ${KIT_LIBS} ${MANGLED_MESA_LIBRARY})
    IF(MANGLED_OSMESA_LIBRARY)
      SET( KIT_LIBS ${KIT_LIBS} ${MANGLED_OSMESA_LIBRARY} )
    ENDIF(MANGLED_OSMESA_LIBRARY)
  ENDIF (MANGLED_MESA_LIBRARY)
ELSE (VTK_USE_MANGLED_MESA)
  IF(VTK_OPENGL_HAS_OSMESA)
    IF (OSMESA_LIBRARY)
      SET(KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY})
    ENDIF (OSMESA_LIBRARY)
  ENDIF(VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_USE_MANGLED_MESA)
SET(Kit_SRCS ${Kit_SRCS} ${KitOpenGL_SRCS})
SET(Kit_EXTRA_SRCS ${Kit_FT_SRCS})

IF(VTK_USE_GL2PS)
SET(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${Kit_GL2PS_SRCS})
ENDIF(VTK_USE_GL2PS)

SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(Kit_TCL_EXTRA_CMDS)

IF (VTK_WRAP_TCL)
  IF(NOT VTK_USE_COCOA)
    SET(KitTCL_SRCS
       vtkTkRenderWidget.cxx
       vtkTkImageViewerWidget.cxx)
  ENDIF(NOT VTK_USE_COCOA)

  # find the tk internals for win32
  IF (NOT WIN32)
    IF(NOT APPLE)
      SET(KitTCL_SRCS ${KitTCL_SRCS} 
        vtkXRenderWindowTclInteractor.cxx)
    ENDIF (NOT APPLE)
  ENDIF (NOT WIN32) 
  
  IF (NOT VTK_DISABLE_TK_INIT)
   SET(Kit_TCL_EXTRA_CMDS 
     vtkTkRenderWidget 
     vtkTkImageViewerWidget)
  ENDIF (NOT VTK_DISABLE_TK_INIT)
  IF (APPLE AND VTK_BUILD_SHARED_LIBS)
    SET(Kit_TCL_EXTRA_SRCS ${Kit_TCL_EXTRA_SRCS} vtkOSXRenderingTclInit.c)
  ENDIF (APPLE AND VTK_BUILD_SHARED_LIBS)
ENDIF (VTK_WRAP_TCL)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_${UKIT}_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_init${KIT}")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtk${KIT}TCL SOURCES KitTCL_SRCS 
                ${Kit_SRCS}
                COMMANDS ${Kit_TCL_EXTRA_CMDS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS} ${PYTHON_LIBRARY})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    INSTALL_TARGETS(/lib/vtk vtk${KIT}PythonD)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)

# End of common section
# -----------------------------------------------------------------


IF (VTK_WRAP_TCL)
  IF (APPLE AND VTK_BUILD_SHARED_LIBS)
    IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
      SET_TARGET_PROPERTIES(vtk${KIT}TCL PROPERTIES LINK_FLAGS "-init _oft_initRenOSXInit")
    ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  ENDIF (APPLE AND VTK_BUILD_SHARED_LIBS)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
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
                              vtk${KIT}
                              ${TCL_LIBRARY}
                              ${TK_LIBRARY})
       INSTALL_TARGETS(/lib/vtk vtkRenderingPythonTkWidgets)
     ENDIF(NOT VTK_USE_COCOA)
  ENDIF (TK_LIBRARY)
ENDIF (VTK_WRAP_PYTHON)

IF(VTK_USE_X)
  TARGET_LINK_LIBRARIES(vtk${KIT} -lXt ${CMAKE_X_LIBS})
  ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(VTK_USE_X)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/${KIT}/tkInternals/.NoDartCoverage
${VTK_BINARY_DIR}/${KIT}/tkInternals/.NoDartCoverage)

IF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_INTERNAL_PATH)
  IF("${TK_INTERNAL_PATH}" MATCHES "${KIT}/tkInternals/tk")
    SUBDIRS(tkInternals)
  ENDIF("${TK_INTERNAL_PATH}" MATCHES "${KIT}/tkInternals/tk")
ENDIF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_INTERNAL_PATH)
