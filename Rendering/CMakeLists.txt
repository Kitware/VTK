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
  vtkAbstractPicker.cxx
  vtkAbstractPropPicker.cxx
  vtkAbstractVolumeMapper.cxx
  vtkActorCollection.cxx
  vtkActor.cxx
  vtkAreaPicker.cxx
  vtkAssembly.cxx
  vtkAxisActor2D.cxx
  vtkCamera.cxx
  vtkCameraInterpolator.cxx
  vtkCellCenterDepthSort.cxx
  vtkCellPicker.cxx
  vtkChooserPainter.cxx
  vtkClipPlanesPainter.cxx
  vtkCoincidentTopologyResolutionPainter.cxx
  vtkCompositePainter.cxx
  vtkCompositePolyDataMapper2.cxx
  vtkCompositePolyDataMapper.cxx
  vtkCullerCollection.cxx
  vtkCuller.cxx
  vtkDataSetMapper.cxx
  vtkDefaultPainter.cxx
  vtkDisplayListPainter.cxx
  vtkDynamic2DLabelMapper.cxx
  vtkExporter.cxx
  vtkFollower.cxx
  vtkFrustumCoverageCuller.cxx
  vtkGenericRenderWindowInteractor.cxx
  vtkGenericVertexAttributeMapping.cxx
  vtkGraphicsFactory.cxx
  vtkHierarchicalPolyDataMapper.cxx
  vtkIdentColoredPainter.cxx
  vtkImageActor.cxx
  vtkImageMapper.cxx
  vtkImageViewer2.cxx
  vtkImageViewer.cxx
  vtkImagingFactory.cxx
  vtkImporter.cxx
  vtkInteractorEventRecorder.cxx
  vtkInteractorObserver.cxx
  vtkInteractorStyle.cxx
  vtkInteractorStyleFlight.cxx
  vtkInteractorStyleImage.cxx
  vtkInteractorStyleJoystickActor.cxx
  vtkInteractorStyleJoystickCamera.cxx
  vtkInteractorStyleRubberBand2D.cxx
  vtkInteractorStyleRubberBand3D.cxx
  vtkInteractorStyleRubberBandPick.cxx
  vtkInteractorStyleRubberBandZoom.cxx
  vtkInteractorStyleSwitch.cxx
  vtkInteractorStyleTerrain.cxx
  vtkInteractorStyleTrackballActor.cxx
  vtkInteractorStyleTrackballCamera.cxx
  vtkInteractorStyleTrackball.cxx
  vtkInteractorStyleUnicam.cxx
  vtkInteractorStyleUser.cxx
  vtkIVExporter.cxx
  vtkLabeledDataMapper.cxx
  vtkLeaderActor2D.cxx
  vtkLightCollection.cxx
  vtkLight.cxx
  vtkLightingPainter.cxx
  vtkLightKit.cxx
  vtkLinesPainter.cxx
  vtkLODActor.cxx
  vtkLODProp3D.cxx
  vtkMapperCollection.cxx
  vtkMapper.cxx
  vtkOBJExporter.cxx
  vtkObserverMediator.cxx
  vtkOOGLExporter.cxx
  vtkPainter.cxx
  vtkPainterDeviceAdapter.cxx
  vtkPainterPolyDataMapper.cxx
  vtkParallelCoordinatesActor.cxx
  vtkPicker.cxx
  vtkPointPicker.cxx
  vtkPointsPainter.cxx
  vtkPOVExporter.cxx
  vtkPolyDataMapper2D.cxx
  vtkPolyDataMapper.cxx
  vtkPolyDataPainter.cxx
  vtkPolygonsPainter.cxx
  vtkPrimitivePainter.cxx
  vtkProp3DCollection.cxx
  vtkProp3D.cxx
  vtkProperty.cxx
  vtkPropPicker.cxx
  vtkQuadricLODActor.cxx
  vtkQuaternionInterpolator.cxx
  vtkRenderedAreaPicker.cxx
  vtkRendererCollection.cxx
  vtkRenderer.cxx
  vtkRendererSource.cxx
  vtkRenderWindowCollection.cxx
  vtkRenderWindow.cxx
  vtkRenderWindowInteractor.cxx
  vtkRepresentationPainter.cxx
  vtkScalarBarActor.cxx
  vtkScalarsToColorsPainter.cxx
  vtkScaledTextActor.cxx
  vtkScenePicker.cxx
  vtkSelectVisiblePoints.cxx
  vtkShader.cxx
  vtkShaderDeviceAdapter.cxx
  vtkShaderProgram.cxx
  vtkStandardPolyDataPainter.cxx
  vtkTesting.cxx
  # vtkTextActor2D.cxx
  vtkTextActor3D.cxx
  vtkTextActor.cxx
  vtkTextMapper.cxx
  vtkTextProperty.cxx
  vtkTexture.cxx
  vtkTexturedActor2D.cxx
  vtkTransformInterpolator.cxx
  vtkTStripsPainter.cxx
  vtkTupleInterpolator.cxx
  vtkVisibilitySort.cxx
  vtkVisibleCellSelector.cxx
  vtkVolumeCollection.cxx
  vtkVolume.cxx
  vtkVolumeProperty.cxx
  vtkVRMLExporter.cxx
  vtkWindowToImageFilter.cxx
  vtkWorldPointPicker.cxx
  )

IF(VTK_USE_GL2PS)
  SET(Kit_SRCS ${Kit_SRCS} vtkGL2PSExporter.cxx)
  SET(Kit_GL2PS_SRCS ${VTK_SOURCE_DIR}/Utilities/gl2ps/gl2ps.c)
  ADD_DEFINITIONS(-DGL2PS_HAVE_ZLIB)
  ADD_DEFINITIONS(-DGL2PS_HAVE_LIBPNG)
ENDIF(VTK_USE_GL2PS)

IF(VTK_USE_CG_SHADERS)
  SET(Kit_SRCS ${Kit_SRCS} 
    vtkCgShader.cxx
    vtkCgShaderDeviceAdapter.cxx
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
    vtkGLSLShaderDeviceAdapter.cxx
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
  vtkShaderDeviceAdapter
  vtkShaderProgram
  vtkVisibilitySort.cxx
  ABSTRACT
  )

# Freetype stuff

SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkOpenGLFreeTypeTextMapper.cxx)

SET(Kit_SRCS ${Kit_SRCS} vtkFreeTypeUtilities.cxx )
SET_SOURCE_FILES_PROPERTIES(vtkFreeTypeUtilities WRAP_EXCLUDE)

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
  SET( Kit_SRCS ${Kit_SRCS} vtkXRenderWindowInteractor.cxx)
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
    INCLUDE(CheckFunctionExists)
    SET(VTK_SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    SET(CMAKE_REQUIRED_LIBRARIES
      ${OPENGL_LIBRARIES}
      ${CMAKE_THREAD_LIBS}
      ${CMAKE_REQUIRED_LIBRARIES}
      -lm
      )
    IF(VTK_USE_X)
      CHECK_FUNCTION_EXISTS(glXGetProcAddressARB VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
      IF (VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
        IF (NOT "${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES}" STREQUAL "${OPENGL_INCLUDE_DIR}")
          # The OpenGL includes changed.  Retest glXGetProcAddressARB prototype.
          SET(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED 0)
        ENDIF (NOT "${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES}" STREQUAL "${OPENGL_INCLUDE_DIR}")
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
    ELSE(VTK_USE_X)
      SET(VTK_NO_EXTENSION_LOADING 1)
    ENDIF(VTK_USE_X)
    SET(CMAKE_REQUIRED_LIBRARIES "${VTK_SAVE_CMAKE_REQUIRED_LIBRARIES}")
  ENDIF (APPLE)
ENDIF (WIN32)
CONFIGURE_FILE(
  ${VTK_SOURCE_DIR}/Rendering/vtkOpenGLExtensionManagerConfigure.h.in
  ${VTK_BINARY_DIR}/Rendering/vtkOpenGLExtensionManagerConfigure.h)

# Build a list of libraries to link to vtkRendering.

IF(VTK_USE_GL2PS)
  SET(KIT_LIBS ${KIT_LIBS} ${VTK_ZLIB_LIBRARIES} ${VTK_PNG_LIBRARIES})
ENDIF(VTK_USE_GL2PS)

IF (WIN32)
  IF(VTK_USE_X)
    IF (OPENGL_gl_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkXOpenGLRenderWindow.cxx)
      SET(KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} )
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
                   vtkCocoaGLView.mm)
       
      # The objective-c++ classes might need their own flags.
      # Set them if specified.
      IF(VTK_REQUIRED_OBJCXX_FLAGS)

        SET_SOURCE_FILES_PROPERTIES(vtkCocoaRenderWindowInteractor.mm 
                                    vtkCocoaRenderWindow.mm 
                                    vtkCocoaGLView.mm
                                    PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")

      ENDIF(VTK_REQUIRED_OBJCXX_FLAGS)

      SET_SOURCE_FILES_PROPERTIES(vtkCocoaGLView WRAP_EXCLUDE)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} "-framework Cocoa")
    ENDIF(VTK_USE_COCOA)
    IF(VTK_USE_CARBON)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkCarbonRenderWindowInteractor.cxx
                   vtkCarbonRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} "-framework Carbon")
    ENDIF(VTK_USE_CARBON)
    IF(VTK_USE_X  AND  OPENGL_gl_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkXOpenGLRenderWindow.cxx)
    ENDIF(VTK_USE_X  AND  OPENGL_gl_LIBRARY)
  ELSE (APPLE)
    IF (VTK_USE_X AND OPENGL_gl_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkXOpenGLRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} )
    ELSE (VTK_USE_X AND OPENGL_gl_LIBRARY)
      IF (VTK_OPENGL_HAS_OSMESA  AND  OSMESA_LIBRARY)
        SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkOSOpenGLRenderWindow.cxx)
        SET (KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY} )
      ENDIF (VTK_OPENGL_HAS_OSMESA  AND  OSMESA_LIBRARY)
    ENDIF (VTK_USE_X AND OPENGL_gl_LIBRARY)
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
  IF(VTK_OPENGL_HAS_OSMESA  AND  OSMESA_LIBRARY)
    SET(KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY})
  ENDIF(VTK_OPENGL_HAS_OSMESA  AND  OSMESA_LIBRARY)
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

IF (TK_FOUND AND VTK_USE_TK)
  SET(VTK_GENERATE_TK_INTERNALS 1)
ENDIF (TK_FOUND AND VTK_USE_TK)

IF(VTK_GENERATE_TK_INTERNALS)
  SET (HAVE_LIMITS_H ${CMAKE_HAVE_LIMITS_H})
  SET (HAVE_UNISTD_H ${CMAKE_HAVE_UNISTD_H})
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkTkInternals.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
      COMPONENT Development
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ENDIF (VTK_GENERATE_TK_INTERNALS)

IF (VTK_WRAP_TCL AND VTK_USE_TK)
  SET(KitTCL_SRCS
    vtkTkRenderWidget.cxx
    vtkTkImageViewerWidget.cxx)

  # Use special interactor for X and Tk.
  IF(VTK_USE_X)
    SET(KitTCL_SRCS ${KitTCL_SRCS} vtkXRenderWindowTclInteractor.cxx)
  ENDIF(VTK_USE_X)
  SET(Kit_TCL_EXTRA_CMDS 
    vtkTkRenderWidget 
    vtkTkImageViewerWidget)
ENDIF (VTK_WRAP_TCL AND VTK_USE_TK)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

# when crosscompiling vtkParseOGLExt is imported, don't add a dependency to it
# this should be handled by cmake so it doesn't have to be done here
IF(NOT CMAKE_CROSSCOMPILING)
  # Add target dependency to ensure the vtkParseOGLExt is built before
  # Rendering is.
  ADD_DEPENDENCIES(vtk${KIT} vtkParseOGLExt)
ENDIF(NOT CMAKE_CROSSCOMPILING)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGL.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

IF (VTK_WRAP_PYTHON AND VTK_USE_TK AND TK_FOUND)
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
    INSTALL(TARGETS vtkRenderingPythonTkWidgets
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_PYTHON AND VTK_USE_TK AND TK_FOUND)

IF(VTK_USE_X)
  SET(X11_LIB_SEARCH_PATH
    /usr/X11R6/lib
    /usr/local/lib 
    /usr/openwin/lib 
    /usr/lib 
    )
  FIND_LIBRARY(X11_Xt_LIB Xt ${X11_LIB_SEARCH_PATH})
  MARK_AS_ADVANCED(X11_Xt_LIB)
  IF (X11_Xt_LIB)
    TARGET_LINK_LIBRARIES(vtk${KIT} ${X11_Xt_LIB})
  ENDIF (X11_Xt_LIB)
  TARGET_LINK_LIBRARIES(vtk${KIT} ${X11_LIBRARIES})
  IF (VTK_USE_TK)
    # Definitely needed in case we are linking against a Tcl/Tk 8.5 static,
    # which by default seems to be missing Xss and Xft. It can not just be
    # attached to the vtk interpreter because sub-projects may need the
    # very same fix.
    FIND_LIBRARY(X11_Xscreensaver_LIB Xss ${X11_LIB_SEARCH_PATH})
    MARK_AS_ADVANCED(X11_Xscreensaver_LIB)
    IF(X11_Xscreensaver_LIB)
      TARGET_LINK_LIBRARIES(vtk${KIT} ${X11_Xscreensaver_LIB})
    ENDIF(X11_Xscreensaver_LIB)
    FIND_LIBRARY(X11_Xft_LIB Xft ${X11_LIB_SEARCH_PATH})
    MARK_AS_ADVANCED(X11_Xft_LIB)
    IF(X11_Xft_LIB)
      TARGET_LINK_LIBRARIES(vtk${KIT} ${X11_Xft_LIB})
    ENDIF(X11_Xft_LIB)
    IF(APPLE)
      FIND_LIBRARY(X11_fontconfig_LIB fontconfig ${X11_LIB_SEARCH_PATH})
      MARK_AS_ADVANCED(X11_fontconfig_LIB)
      IF(X11_fontconfig_LIB)
        TARGET_LINK_LIBRARIES(vtk${KIT} ${X11_fontconfig_LIB})
      ENDIF(X11_fontconfig_LIB)
    ENDIF(APPLE)
  ENDIF(VTK_USE_TK)
ENDIF(VTK_USE_X)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkRegressionTestImage.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
