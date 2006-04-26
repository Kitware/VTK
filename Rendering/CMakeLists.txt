SET(KIT Rendering)
SET(UKIT RENDERING)
SET(KIT_TCL_LIBS vtkGraphicsTCL vtkImagingTCL ${VTK_TK_LIBRARIES})
SET(KIT_PYTHON_LIBS vtkGraphicsPythonD vtkImagingPythonD)
SET(KIT_JAVA_LIBS vtkGraphicsJava vtkImagingJava)
IF (JAVA_AWT_LIBRARY)
  SET(KIT_JAVA_LIBS ${KIT_JAVA_LIBS} ${JAVA_AWT_LIBRARY})
ENDIF (JAVA_AWT_LIBRARY)
SET(KIT_LIBS vtkGraphics vtkImaging vtkIO
  vtkftgl
  ${VTK_FREETYPE_LIBRARIES}
)

# Fix for bug#1026 moved out of the common section.  This should be
# changed to add the flag only in the right places (CMAKE_CXX_FLAGS?).
IF(APPLE AND VTK_WRAP_JAVA)
  ADD_DEFINITIONS("-ObjC++")
ENDIF(APPLE AND VTK_WRAP_JAVA)

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
vtkCameraInterpolator.cxx
vtkCellCenterDepthSort.cxx
vtkCellPicker.cxx
vtkChooserPainter.cxx
vtkClipPlanesPainter.cxx
vtkCoincidentTopologyResolutionPainter.cxx
vtkCuller.cxx
vtkCullerCollection.cxx
vtkDataSetMapper.cxx
vtkDefaultPainter.cxx
vtkDisplayListPainter.cxx
vtkExporter.cxx
vtkFollower.cxx
vtkFrustumCoverageCuller.cxx
vtkGenericRenderWindowInteractor.cxx
vtkGraphicsFactory.cxx
vtkHierarchicalPolyDataMapper.cxx
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
vtkLeaderActor2D.cxx
vtkLight.cxx
vtkLightCollection.cxx
vtkLightKit.cxx
vtkLightingPainter.cxx
vtkLinesPainter.cxx
vtkMapper.cxx
vtkMapperCollection.cxx
vtkMultiGroupPolyDataMapper.cxx
vtkOBJExporter.cxx
vtkObserverMediator.cxx
vtkOOGLExporter.cxx
vtkPainter.cxx
vtkPainterDeviceAdapter.cxx
vtkPainterPolyDataMapper.cxx
vtkParallelCoordinatesActor.cxx
vtkPicker.cxx
vtkPointsPainter.cxx
vtkPointPicker.cxx
vtkPolyDataMapper.cxx
vtkPolyDataMapper2D.cxx
vtkPolyDataPainter.cxx
vtkPolygonsPainter.cxx
vtkPrimitivePainter.cxx
vtkProp3D.cxx
vtkProp3DCollection.cxx
vtkPropPicker.cxx
vtkProperty.cxx
vtkQuaternionInterpolator.cxx
vtkRenderWindow.cxx
vtkRenderWindowCollection.cxx
vtkRenderWindowInteractor.cxx
vtkRenderer.cxx
vtkRendererCollection.cxx
vtkRendererSource.cxx
vtkRepresentationPainter.cxx
vtkScalarBarActor.cxx
vtkScalarsToColorsPainter.cxx
vtkScaledTextActor.cxx
vtkSelectVisiblePoints.cxx
vtkShader.cxx
vtkShaderProgram.cxx
vtkStandardPolyDataPainter.cxx
vtkTesting.cxx
vtkTextActor.cxx
# vtkTextActor2D.cxx
vtkTextActor3D.cxx
vtkTextMapper.cxx
vtkTextProperty.cxx
vtkTexture.cxx
vtkTransformInterpolator.cxx
vtkTStripsPainter.cxx
vtkTupleInterpolator.cxx
vtkVisibilitySort.cxx
vtkVRMLExporter.cxx
vtkVolume.cxx
vtkVolumeCollection.cxx
vtkVolumeProperty.cxx
vtkWindowToImageFilter.cxx
vtkWorldPointPicker.cxx
vtkInteractorStyleRubberBandPick.cxx
vtkAreaPicker.cxx
vtkRenderedAreaPicker.cxx
)

IF(VTK_USE_GL2PS)
  SET(Kit_SRCS ${Kit_SRCS} vtkGL2PSExporter.cxx)
  SET(Kit_GL2PS_SRCS ${VTK_SOURCE_DIR}/Utilities/gl2ps/gl2ps.c)
  ADD_DEFINITIONS(-DGL2PS_HAVE_ZLIB)
ENDIF(VTK_USE_GL2PS)

IF(VTK_USE_CG_SHADERS)
  SET(Kit_SRCS ${Kit_SRCS} 
    vtkCgShader.cxx
    vtkCgShaderProgram.cxx
    )
  INCLUDE_DIRECTORIES(${CG_INCLUDE_PATH})
  SET (KIT_LIBS ${KIT_LIBS} ${CG_LIBRARY} ${CG_GL_LIBRARY})
ENDIF(VTK_USE_CG_SHADERS)


SET( KitOpenGL_SRCS
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLClipPlanesPainter.cxx
  vtkOpenGLCoincidentTopologyResolutionPainter.cxx
  vtkOpenGLDisplayListPainter.cxx
  vtkOpenGLExtensionManager.cxx
  vtkOpenGLImageActor.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLLightingPainter.cxx
  vtkOpenGLPainterDeviceAdapter.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLRepresentationPainter.cxx
  vtkOpenGLScalarsToColorsPainter.cxx
  vtkOpenGLTexture.cxx
  )

IF(VTK_USE_GLSL_SHADERS)
  SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
    vtkGLSLShader.cxx
    vtkGLSLShaderProgram.cxx
    )
ENDIF(VTK_USE_GLSL_SHADERS)

SET_SOURCE_FILES_PROPERTIES(
vtkAbstractMapper3D
vtkAbstractVolumeMapper
vtkAbstractPicker
vtkAbstractPropPicker
vtkCuller
vtkExporter
vtkImporter
vtkInteractorObserver
vtkMapper
vtkOpenGLRenderWindow
vtkPainter
vtkPolyDataPainter
vtkPrimitivePainter
vtkProp3D
vtkShader
vtkShaderProgram
vtkVisibilitySort.cxx
ABSTRACT
)

# Freetype stuff

SET(Kit_SRCS ${Kit_SRCS}
  vtkFreeTypeUtilities.cxx
)
SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
  vtkOpenGLFreeTypeTextMapper.cxx
)
SET_SOURCE_FILES_PROPERTIES(
  vtkFreeTypeUtilities
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

# OpenGL extensions stuff.

# Parse the extensions into vtkgl.h/vtkgl.cxx.
ADD_CUSTOM_COMMAND(
  OUTPUT ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx
  COMMAND ${VTK_PARSEOGLEXT_EXE}
  ARGS ${VTK_BINARY_DIR}/Rendering ${VTK_GLEXT_FILE} ${VTK_GLXEXT_FILE} ${VTK_WGLEXT_FILE}
  DEPENDS ${VTK_GLEXT_FILE} ${VTK_GLXEXT_FILE} ${VTK_WGLEXT_FILE} ${VTK_PARSEOGLEXT_EXE}
  )

SET_SOURCE_FILES_PROPERTIES(vtkOpenGLExtensionManager.cxx
  PROPERTIES OBJECT_DEPENDS ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx)

SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
  ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  # OK, so it's not abstract, but this stops the instantiator
  # from trying to call vtkRenderingInstantiatorvtkglNew():
  ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx
  GENERATED
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx
  WRAP_EXCLUDE
  )

# Configuration for vtkOpenGLExtensionManager
IF (WIN32)
  SET(VTK_USE_WGL_GET_PROC_ADDRESS 1)
ELSE (WIN32)
  IF (APPLE)
    SET(VTK_USE_APPLE_LOADER 1)
  ELSE (APPLE)
    INCLUDE(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
    SET(VTK_SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    SET(CMAKE_REQUIRED_LIBRARIES
      ${OPENGL_LIBRARIES}
      ${CMAKE_THREAD_LIBS}
      ${CMAKE_REQUIRED_LIBRARIES}
      -lm
      )
    CHECK_FUNCTION_EXISTS(glXGetProcAddressARB VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
    IF (VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
      IF ("${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES}" STREQUAL "${OPENGL_INCLUDE_DIR}")
      ELSE ("${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES}" STREQUAL "${OPENGL_INCLUDE_DIR}")
        # The OpenGL includes changed.  Retest glXGetProcAddressARB prototype.
        SET(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED 0)
      ENDIF ("${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES}" STREQUAL "${OPENGL_INCLUDE_DIR}")
      IF (NOT VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED)
        MESSAGE(STATUS "Looking for glXGetProcAddressARB in GL/glx.h")
        TRY_COMPILE(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS
          ${CMAKE_BINARY_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/CheckglXGetProcAddressARB.cxx
          CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}"
          "-DINCLUDE_DIRECTORIES:STRING=${OPENGL_INCLUDE_DIR}"
          OUTPUT_VARIABLE OUTPUT)
        MESSAGE(STATUS "Looking for glXGetProcAddressARB in GL/glx.h - ${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS}")
        FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeOutput.log
          "Determining if glXGetProcAddressARB exists in GL/glx.h ${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS} with the following output:\n"
          "${OUTPUT}\n\n")
        SET(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED 1
          CACHE INTERNAL "Already set VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS")
        SET(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES
          "${OPENGL_INCLUDE_DIR}"
          CACHE INTERNAL "OpenGL includes used to test glXGetProcAddressARB prototype.")
      ENDIF (NOT VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED)
      IF (NOT VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS)
        SET(VTK_DEFINE_GLX_GET_PROC_ADDRESS_PROTOTYPE 1)
      ENDIF (NOT VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS)
    ELSE (VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
      CHECK_FUNCTION_EXISTS(glXGetProcAddress VTK_USE_GLX_GET_PROC_ADDRESS)
      IF (NOT VTK_USE_GLX_GET_PROC_ADDRESS)
#         SET(VTK_USE_VTK_DYNAMIC_LOADER 1)
        MESSAGE(STATUS "Could not find extension loader.  Extensions disabled.")
        SET(VTK_NO_EXTENSION_LOADING 1)
      ENDIF (NOT VTK_USE_GLX_GET_PROC_ADDRESS)
    ENDIF (VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
    SET(CMAKE_REQUIRED_LIBRARIES "${VTK_SAVE_CMAKE_REQUIRED_LIBRARIES}")
  ENDIF (APPLE)
ENDIF (WIN32)
CONFIGURE_FILE(
  ${VTK_SOURCE_DIR}/Rendering/vtkOpenGLExtensionManagerConfigure.h.in
  ${VTK_BINARY_DIR}/Rendering/vtkOpenGLExtensionManagerConfigure.h)

# Build a list of libraries to link to vtkRendering.

IF(VTK_USE_GL2PS)
  SET(KIT_LIBS ${KIT_LIBS} ${VTK_ZLIB_LIBRARIES})
ENDIF(VTK_USE_GL2PS)

IF (WIN32)
  IF(VTK_USE_X)
    IF (OPENGL_gl_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkXOpenGLRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} )
    ENDIF (OPENGL_gl_LIBRARY)    
  ELSE(VTK_USE_X)
    SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                 vtkWin32OpenGLRenderWindow.cxx
                 vtkWin32RenderWindowInteractor.cxx)
    SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} )
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
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} "-framework Cocoa")
    ENDIF(VTK_USE_COCOA)
    IF(VTK_USE_CARBON)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkCarbonRenderWindowInteractor.cxx
                   vtkCarbonRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} "-framework Carbon")
    ENDIF(VTK_USE_CARBON)
    IF(VTK_USE_X)
      IF (OPENGL_gl_LIBRARY)
        SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                     vtkXOpenGLRenderWindow.cxx)
        SET (KIT_LIBS ${KIT_LIBS} "-lGL")
      ENDIF (OPENGL_gl_LIBRARY)
    ENDIF(VTK_USE_X)
  ELSE (APPLE)
    IF (OPENGL_gl_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkXOpenGLRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} )
    ENDIF (OPENGL_gl_LIBRARY)
  ENDIF (APPLE)
ENDIF (WIN32)
IF(VTK_USE_CG_SHADERS)
  SET(KIT_LIBS ${KIT_LIBS} ${OPENGL_glu_LIBRARY})
ENDIF(VTK_USE_CG_SHADERS)

IF (VTK_USE_MANGLED_MESA)
  IF (MANGLED_MESA_LIBRARY)
    SET ( KitOpenGL_SRCS ${KitOpenGL_SRCS}
                 vtkMesaActor.cxx
                 vtkMesaCamera.cxx
                 vtkMesaClipPlanesPainter.cxx
                 vtkMesaCoincidentTopologyResolutionPainter.cxx
                 vtkMesaDisplayListPainter.cxx
                 vtkMesaImageActor.cxx
                 vtkMesaImageMapper.cxx
                 vtkMesaLight.cxx
                 vtkMesaLightingPainter.cxx
                 vtkMesaPainterDeviceAdapter.cxx
                 vtkMesaPolyDataMapper.cxx
                 vtkMesaPolyDataMapper2D.cxx
                 vtkMesaProperty.cxx
                 vtkMesaRenderWindow.cxx
                 vtkMesaRenderer.cxx
                 vtkMesaRepresentationPainter.cxx
                 vtkMesaScalarsToColorsPainter.cxx
                 vtkMesaTexture.cxx
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
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

IF (TK_LIBRARY AND VTK_USE_TK)
  SET(VTK_GENERATE_TK_INTERNALS 1)
ENDIF (TK_LIBRARY AND VTK_USE_TK)
IF(VTK_GENERATE_TK_INTERNALS)
  SET (HAVE_LIMITS_H ${CMAKE_HAVE_LIMITS_H})
  SET (HAVE_UNISTD_H ${CMAKE_HAVE_UNISTD_H})
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkTkInternals.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}
                  FILES ${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ENDIF (VTK_GENERATE_TK_INTERNALS)

IF (VTK_WRAP_TCL)
  IF(VTK_USE_TK)
    SET(KitTCL_SRCS
      vtkTkRenderWidget.cxx
      vtkTkImageViewerWidget.cxx)

    # Use special interactor for X and Tk.
    IF(VTK_USE_X)
      SET(KitTCL_SRCS ${KitTCL_SRCS}
        vtkXRenderWindowTclInteractor.cxx)
    ENDIF(VTK_USE_X)
    SET(Kit_TCL_EXTRA_CMDS 
      vtkTkRenderWidget 
      vtkTkImageViewerWidget)
  ENDIF (VTK_USE_TK)
ENDIF (VTK_WRAP_TCL)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
# Add target dependency to ensure the vtkParseOGLExt is built before
# Rendering is.
ADD_DEPENDENCIES(vtk${KIT}
  vtkParseOGLExt)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
    vtkOpenGL
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

IF (VTK_WRAP_PYTHON)
  IF(VTK_USE_TK)
    IF (TK_LIBRARY)
       SET(RenderingPythonTkWidgets_SRCS
          vtkTkWidgetsInit.cxx
          vtkTkRenderWidgetPython.cxx
          vtkTkImageViewerWidgetPython.cxx
          )
       ADD_LIBRARY(vtkRenderingPythonTkWidgets SHARED
                   ${RenderingPythonTkWidgets_SRCS})
       TARGET_LINK_LIBRARIES (vtkRenderingPythonTkWidgets
                              vtk${KIT}
                              ${VTK_TK_LIBRARIES})

       # Apply user-defined properties to the library target.
       IF(VTK_LIBRARY_PROPERTIES)
         SET_TARGET_PROPERTIES(vtkRenderingPythonTkWidgets PROPERTIES
           ${VTK_LIBRARY_PROPERTIES})
       ENDIF(VTK_LIBRARY_PROPERTIES)

       IF(NOT VTK_INSTALL_NO_LIBRARIES)
         INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtkRenderingPythonTkWidgets)
       ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
     ENDIF (TK_LIBRARY)
   ENDIF(VTK_USE_TK)
ENDIF (VTK_WRAP_PYTHON)

IF(VTK_USE_X)
  TARGET_LINK_LIBRARIES(vtk${KIT} -lXt ${X11_LIBRARIES})
ENDIF(VTK_USE_X)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} FILES
                ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkRegressionTestImage.h)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
