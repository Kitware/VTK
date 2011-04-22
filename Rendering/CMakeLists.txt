SET(KIT Rendering)
SET(UKIT RENDERING)
SET(KIT_TCL_LIBS vtkGraphicsTCL vtkImagingTCL ${VTK_TK_LIBRARIES})
SET(KIT_PYTHON_LIBS vtkGraphicsPythonD vtkImagingPythonD)
SET(KIT_JAVA_LIBS vtkGraphicsJava vtkImagingJava)

IF (JAVA_AWT_LIBRARY)
  SET(KIT_JAVA_LIBS ${KIT_JAVA_LIBS} ${JAVA_AWT_LIBRARY})
ENDIF (JAVA_AWT_LIBRARY)

SET(KIT_INTERFACE_LIBRARIES vtkGraphics vtkImaging)
SET(KIT_LIBS vtkIO vtkftgl
  ${VTK_FREETYPE_LIBRARIES}
)
#INCLUDE(${VTK_CMAKE_DIR}/vtkTestGL.cmake)
#INCLUDE(${VTK_CMAKE_DIR}/vtkTestGLX.cmake)




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
  vtkMapArrayValues.cxx
  vtkAssembly.cxx
  vtkAxisActor2D.cxx
  vtkCamera.cxx
  vtkCameraActor.cxx
  vtkCameraInterpolator.cxx
  vtkCellCenterDepthSort.cxx
  vtkCellPicker.cxx
  vtkChooserPainter.cxx
  vtkClipPlanesPainter.cxx
  vtkCoincidentTopologyResolutionPainter.cxx
  vtkColorMaterialHelper.cxx
  vtkCompositePainter.cxx
  vtkCompositePolyDataMapper2.cxx
  vtkCompositePolyDataMapper.cxx
  vtkCullerCollection.cxx
  vtkCuller.cxx
  vtkDataSetMapper.cxx
  vtkDataTransferHelper.cxx
  vtkDefaultPainter.cxx
  vtkDisplayListPainter.cxx
  vtkDistanceToCamera.cxx
  vtkDynamic2DLabelMapper.cxx
  vtkDummyGPUInfoList.cxx
  vtkExporter.cxx
  vtkFollower.cxx
  vtkFrameBufferObject.cxx
  vtkFreeTypeStringToImage.cxx
  vtkFrustumCoverageCuller.cxx
  vtkGenericRenderWindowInteractor.cxx
  vtkGenericVertexAttributeMapping.cxx
  vtkGlyph3DMapper.cxx
  vtkGPUInfo.cxx
  vtkGPUInfoList.cxx
  vtkGraphicsFactory.cxx
  vtkGraphMapper.cxx
  vtkGraphToGlyphs.cxx
  vtkHardwareSelector.cxx
  vtkHardwareSelectionPolyDataPainter.cxx
  vtkHierarchicalPolyDataMapper.cxx
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
  vtkLabeledTreeMapDataMapper.cxx
  vtkLabelHierarchy.cxx
  vtkLabelHierarchyAlgorithm.cxx
  vtkLabelHierarchyIterator.cxx
  vtkLabelHierarchyCompositeIterator.cxx
  vtkLabelPlacer.cxx
  vtkLabelPlacementMapper.cxx
  vtkLabelRenderStrategy.cxx
  vtkLabelSizeCalculator.cxx
  vtkLeaderActor2D.cxx
  vtkLightActor.cxx
  vtkLightCollection.cxx
  vtkLight.cxx
  vtkLightingHelper.cxx
  vtkLightingPainter.cxx
  vtkLightKit.cxx
  vtkLineIntegralConvolution2D.cxx
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
  vtkParallelCoordinatesInteractorStyle.cxx
  vtkPicker.cxx
  vtkPixelBufferObject.cxx
  vtkPointPicker.cxx
  vtkPointSetToLabelHierarchy.cxx
  vtkPointsPainter.cxx
  vtkPOVExporter.cxx
  vtkPolyDataMapper2D.cxx
  vtkPolyDataMapper.cxx
  vtkPolyDataPainter.cxx
  vtkPolygonsPainter.cxx
  vtkPrimitivePainter.cxx
  vtkProp3DCollection.cxx
  vtkProp3D.cxx
  vtkProp3DFollower.cxx
  vtkProperty.cxx
  vtkPropPicker.cxx
  vtkQuadricLODActor.cxx
  vtkQuaternionInterpolator.cxx
  vtkRenderedAreaPicker.cxx
  vtkRendererCollection.cxx
  vtkRenderer.cxx
  vtkRendererDelegate.cxx
  vtkRendererSource.cxx
  vtkRenderPass.cxx
  vtkRenderState.cxx
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
  vtkStringToImage.cxx
  vtkSurfaceLICDefaultPainter.cxx
  vtkSurfaceLICPainter.cxx
  vtkTDxInteractorStyle.cxx
  vtkTDxInteractorStyleCamera.cxx
  vtkTDxInteractorStyleGeo.cxx
  vtkTDxInteractorStyleSettings.cxx
  vtkTesting.cxx
  vtkTestingInteractor.cxx
  vtkTestingObjectFactory.cxx
  # vtkTextActor2D.cxx
  vtkTextActor3D.cxx
  vtkTextActor.cxx
  vtkTextMapper.cxx
  vtkTextProperty.cxx
  vtkTexture.cxx
  vtkTexturedActor2D.cxx
  vtkTextureObject.cxx
  vtkTransformInterpolator.cxx
  vtkTStripsPainter.cxx
  vtkTupleInterpolator.cxx
  vtkViewTheme.cxx
  vtkVisibilitySort.cxx
  vtkVolumeCollection.cxx
  vtkVolume.cxx
  vtkVolumeProperty.cxx
  vtkVRMLExporter.cxx
  vtkWindowToImageFilter.cxx
  vtkWorldPointPicker.cxx
  )

IF (NOT VTK_LEGACY_REMOVE)
  SET (Kit_Deprecated_SRCS
    vtkVisibleCellSelector.cxx
    vtkIdentColoredPainter.cxx)

  SET (Kit_SRCS ${Kit_SRCS} ${Kit_Deprecated_SRCS})
ENDIF (NOT VTK_LEGACY_REMOVE)

IF(VTK_USE_GL2PS)
  SET(Kit_SRCS ${Kit_SRCS} vtkGL2PSExporter.cxx)

  IF(VTK_USE_SYSTEM_GL2PS)
  ELSE(VTK_USE_SYSTEM_GL2PS)
    SET(Kit_GL2PS_SRCS ${VTK_SOURCE_DIR}/Utilities/vtkgl2ps/gl2ps.c)
    ADD_DEFINITIONS(-DGL2PS_HAVE_ZLIB)
    ADD_DEFINITIONS(-DGL2PS_HAVE_LIBPNG)
  ENDIF(VTK_USE_SYSTEM_GL2PS)
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
  vtkCameraPass.cxx
  vtkClearZPass.cxx
  vtkDefaultPass.cxx
  vtkDepthPeelingPass.cxx
  vtkGaussianBlurPass.cxx
  vtkImageProcessingPass.cxx
  vtkLightsPass.cxx
  vtkGLSLShaderDeviceAdapter2.cxx
  vtkOpaquePass.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLClipPlanesPainter.cxx
  vtkOpenGLCoincidentTopologyResolutionPainter.cxx
  vtkOpenGLDisplayListPainter.cxx
  vtkOpenGLExtensionManager.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSupport.cxx
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
  vtkOpenGLState.cxx
  vtkOpenGLTexture.cxx
  vtkOverlayPass.cxx
  vtkRenderPassCollection.cxx
  vtkSequencePass.cxx
  vtkShader2.cxx
  vtkShader2Collection.cxx
  vtkShaderProgram2.cxx
  vtkShadowMapBakerPass.cxx
  vtkShadowMapPass.cxx
  vtkSobelGradientMagnitudePass.cxx
  vtkTextureUnitManager.cxx
  vtkTranslucentPass.cxx
  vtkUniformVariables.cxx
  vtkVolumetricPass.cxx
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
  vtkGPUInfoList
  vtkImageProcessingPass
  vtkImporter
  vtkInteractorObserver
  vtkLabelRenderStrategy
  vtkMapper
  vtkOpenGLRenderWindow
  vtkPainter
  vtkPolyDataPainter
  vtkPrimitivePainter
  vtkProp3D
  vtkRendererDelegate
  vtkRenderPass
  vtkRenderState # not abstract just not using instantiator
  vtkShader
  vtkShaderDeviceAdapter
  vtkShaderProgram
  vtkStringToImage
  vtkTDxInteractorStyle
  vtkVisibilitySort.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkLightingHelper
  vtkRenderState
  vtkOpenGLState
  vtkShader2
  vtkShader2Collection
  vtkShaderProgram2
  vtkTextureUnitManager
  vtkTestingInteractor
  vtkTestingObjectFactory
  WRAP_EXCLUDE
  )

# Freetype stuff

SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
  vtkFreeTypeLabelRenderStrategy.cxx
  vtkOpenGLFreeTypeTextMapper.cxx
  )

# Get GPU VRAM info
# on Windows
#message("DirectX_FOUND=${DirectX_FOUND}")
if(DirectX_FOUND)
 set(Kit_SRCS ${Kit_SRCS} vtkDirectXGPUInfoList.cxx)
endif(DirectX_FOUND)

# on Mac
if(ApplicationServices_FOUND)
 set(Kit_SRCS ${Kit_SRCS} vtkCoreGraphicsGPUInfoList.cxx)
endif()

# on Linux, for Nvidia
if(VTK_USE_X)
 set(Kit_SRCS ${Kit_SRCS} vtkXGPUInfoList.cxx)
endif()

SET(Kit_SRCS ${Kit_SRCS} vtkFreeTypeUtilities.cxx vtkFreeTypeTools.cxx )
SET_SOURCE_FILES_PROPERTIES(
  vtkFreeTypeTools
  vtkFreeTypeUtilities
  WRAP_EXCLUDE)

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

# -----------------------------------------------------------------------------
# 3DConnexion device (SpaceNavigator, etc... ) on Unix.
IF(VTK_USE_X)
  OPTION(VTK_USE_TDX "Use 3Dconnexion device" OFF)
  MARK_AS_ADVANCED(VTK_USE_TDX)
  IF(VTK_USE_TDX)

   SET(VTK_TDX_INCLUDE_PATH CACHE PATH "Directory path to xdrvlib.h for 3DConnexion device")
   SET(VTK_TDX_OBJECT_PATH CACHE FILEPATH "File path to xdrvlib.a for 3DConnexion device")
   SET(Kit_SRCS ${Kit_SRCS} vtkTDxDevice.cxx vtkTDxUnixDevice.cxx)
   INCLUDE_DIRECTORIES(${VTK_TDX_INCLUDE_PATH})
   SET_SOURCE_FILES_PROPERTIES(vtkTDxDevice ABSTRACT)
   SET_SOURCE_FILES_PROPERTIES(vtkTDxUnixDevice WRAP_EXCLUDE)
   LIST(APPEND KIT_INTERFACE_LIBRARIES ${VTK_TDX_OBJECT_PATH})
  ENDIF()
ENDIF()
# -----------------------------------------------------------------------------
# 3DConnexion device (SpaceNavigator, etc... ) on Mac (not X11).
IF(VTK_USE_CARBON OR VTK_USE_COCOA)
  OPTION(VTK_USE_TDX "Use 3Dconnexion device" OFF)
  MARK_AS_ADVANCED(VTK_USE_TDX)
  IF(VTK_USE_TDX)
# framework is in /Library/Frameworks/3DconnexionClient.framework
   find_package(3DconnexionClient)
   if(3DconnexionClient_FOUND)
     set(KIT_LIBS ${KIT_LIBS} ${3DconnexionClient_LIBRARY})
     include_directories(${3DconnexionClient_INCLUDE_DIR})
   endif()
   SET(Kit_SRCS ${Kit_SRCS} vtkTDxDevice.cxx vtkTDxMacDevice.cxx)
   SET_SOURCE_FILES_PROPERTIES(vtkTDxDevice.cxx ABSTRACT)
   LIST(APPEND KIT_INTERFACE_LIBRARIES ${VTK_TDX_OBJECT_PATH})
  ENDIF()
ENDIF()
# -----------------------------------------------------------------------------
# 3DConnexion device (SpaceNavigator, etc... ) on Windows.
IF(WIN32)
  OPTION(VTK_USE_TDX "Use 3Dconnextion device" OFF)
  MARK_AS_ADVANCED(VTK_USE_TDX)
  IF(VTK_USE_TDX)
   SET(Kit_SRCS ${Kit_SRCS} vtkTDxDevice.cxx vtkTDxWinDevice.cxx)
   SET_SOURCE_FILES_PROPERTIES(vtkTDxDevice.cxx ABSTRACT)
  ENDIF()
ENDIF()
# -----------------------------------------------------------------------------
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Rendering/vtkTDxConfigure.h.in
               ${VTK_BINARY_DIR}/Rendering/vtkTDxConfigure.h @ONLY IMMEDIATE)

IF(VTK_USE_QT)
  SET(QT_DONT_USE_QTGUI)
  INCLUDE(${QT_USE_FILE})

  SET( Kit_SRCS ${Kit_SRCS}
    vtkQImageToImageSource.cxx
    vtkQtInitialization.cxx
    vtkQtLabelRenderStrategy.cxx
    vtkQtStringToImage.cxx
    vtkQtTreeRingLabelMapper.cxx
    )
  # On the Mac, make the QT_LIBRARIES KIT_INTERFACE_LIBRARIES. This is a
  # workaround a problem with the Qt binaries from Trolltech. These libraries
  # use install names that are relative. Then when the linker tries to pull them
  # indirectly (through vtkRendering's list of libraries), it fails to find them.
  # This does not happen when Qt is build from source because it uses absolute
  # path in install names. This workaround fixes the problem by forcing all libraries
  # and executables that depend on vtkRendering to also directly link to Qt libraries.
  IF (APPLE)
    LIST (APPEND KIT_INTERFACE_LIBRARIES ${QT_LIBRARIES})
  ELSE (APPLE)
    LIST (APPEND KIT_LIBS ${QT_LIBRARIES})
  ENDIF (APPLE)

ENDIF(VTK_USE_QT)

# The set of source files to be encoded.
SET(glsl_files
  vtkColorMaterialHelper_vs
  vtkDepthPeeling_fs
  vtkGaussianBlurPassShader_fs
  vtkLighting_s # s not vs or fs because it can be used in both.
  vtkLightingHelper_s # s not vs or fs because it can be used in both.
  vtkLineIntegralConvolution2D_fs
  vtkLineIntegralConvolution2D_fs1
  vtkLineIntegralConvolution2D_fs2
  vtkOpenGLRenderer_PeelingFS
  vtkOpenGLPropertyDefaultPropFunc_fs
  vtkOpenGLPropertyDefaultPropFunc_vs
  vtkOpenGLPropertyDefaultMain_fs
  vtkOpenGLPropertyDefaultMain_vs
  vtkSobelGradientMagnitudePassShader1_fs
  vtkSobelGradientMagnitudePassShader2_fs
  vtkShadowMapPassShader_fs
  vtkShadowMapPassShader_vs
  vtkSurfaceLICPainter_fs1
  vtkSurfaceLICPainter_fs2
  vtkSurfaceLICPainter_vs1
  )

# Create custom commands to encode each assembly file into a C string
# literal in a header file.

SET(glsl_h_files)
FOREACH(file ${glsl_files})
  SET(src ${VTK_SOURCE_DIR}/Rendering/${file}.glsl)
  SET(res ${VTK_BINARY_DIR}/Rendering/${file}.cxx)
  SET(resh ${VTK_BINARY_DIR}/Rendering/${file}.h)
  SET(glsl_h_files ${glsl_h_files} "${resh}")
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file} --build-header VTK_RENDERING_EXPORT vtkSystemIncludes.h
    )
  SET(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${res})
ENDFOREACH(file)

# OpenGL extensions stuff.

IF(VTK_USE_X)
 INCLUDE(${VTK_CMAKE_DIR}/vtkTestGLX.cmake)
ENDIF(VTK_USE_X)

# Parse the extensions into vtkgl.h/vtkgl.cxx.
ADD_CUSTOM_COMMAND(
  OUTPUT ${VTK_BINARY_DIR}/Rendering/vtkgl.cxx
  COMMAND vtkParseOGLExt
  ARGS ${VTK_BINARY_DIR}/Rendering ${VTK_GLEXT_FILE} ${VTK_GLXEXT_FILE} ${VTK_WGLEXT_FILE}
  DEPENDS ${VTK_GLEXT_FILE} ${VTK_GLXEXT_FILE} ${VTK_WGLEXT_FILE} vtkParseOGLExt
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
  vtkLabelHierarchyIterator.cxx
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
  IF(VTK_USE_SYSTEM_GL2PS)
    SET(KIT_LIBS ${KIT_LIBS} ${GL2PS_LIBRARIES} ${VTK_ZLIB_LIBRARIES} ${VTK_PNG_LIBRARIES})
  ELSE(VTK_USE_SYSTEM_GL2PS)
    SET(KIT_LIBS ${KIT_LIBS} ${VTK_ZLIB_LIBRARIES} ${VTK_PNG_LIBRARIES})
  ENDIF(VTK_USE_SYSTEM_GL2PS)
ENDIF(VTK_USE_GL2PS)

IF(OPENGL_gl_LIBRARY)
  SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkGenericOpenGLRenderWindow.cxx)
ENDIF(OPENGL_gl_LIBRARY)

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
      # This is to resolve a linker error that happens on older (?) OS X builds.
      # If an executable links against vtkRendering, it would get the following
      # error unless we add the objc to its link interface:
      # ld: /usr/lib/crt1.o illegal reference to symbol: __objcInit defined in indirectly
      # referenced dynamic library /usr/lib/libobjc.A.dylib
      LIST(APPEND KIT_INTERFACE_LIBRARIES objc)
    ENDIF(VTK_USE_COCOA)
    IF(VTK_USE_CARBON)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS}
                   vtkCarbonRenderWindowInteractor.cxx
                   vtkCarbonRenderWindow.cxx)
      SET (KIT_LIBS ${KIT_LIBS} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} "-framework Carbon")
    ENDIF(VTK_USE_CARBON)
    IF(VTK_USE_X  AND  OPENGL_gl_LIBRARY)
      SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkXOpenGLRenderWindow.cxx)
    ELSE (VTK_USE_X  AND  OPENGL_gl_LIBRARY)
      IF (VTK_OPENGL_HAS_OSMESA  AND  OSMESA_LIBRARY)
        SET(KitOpenGL_SRCS ${KitOpenGL_SRCS} vtkOSOpenGLRenderWindow.cxx)
        SET (KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY} )
      ENDIF (VTK_OPENGL_HAS_OSMESA  AND  OSMESA_LIBRARY)
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

if(DirectX_FOUND)
 # d3d9.lib for symbol Direct3DCreate9
 # symbol CLSID_WbemLocator IID_IWbemLocator: wbemuuid.lib
 # wbemuuid.lib is *NOT A FILE* (you cannot find it), it is probably an
 # embedded library added by the linker itself.
 set(KIT_LIBS ${KIT_LIBS} ${DirectX_LIBRARY} wbemuuid)
 include_directories(${DirectX_INCLUDE_DIR})
endif(DirectX_FOUND)

if(VTK_USE_CORE_GRAPHICS)
 set(KIT_LIBS ${KIT_LIBS} ${ApplicationServices_LIBRARY} ${IOKit_LIBRARY})
 include_directories(${ApplicationServices_INCLUDE_DIR} ${IOKit_INCLUDE_DIR})
endif(VTK_USE_CORE_GRAPHICS)

if(VTK_USE_NVCONTROL)
 # NVCONTROL depends on X11. We solve that by making KIT_LIBS depending on
 # vtkRendering, as vtkRendering depends on X11 on Unix.
 set(KIT_LIBS ${KIT_LIBS} ${NVCtrlLib_LIBRARY} vtkRendering)
 include_directories(${NVCtrlLib_INCLUDE_DIR})
endif(VTK_USE_NVCONTROL)

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
SET(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${Kit_FT_SRCS})

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
  IF (VTK_USE_COCOA)
    SET(KitTCL_SRCS ${KitTCL_SRCS}
      vtkCocoaTkUtilities.mm)
    SET_SOURCE_FILES_PROPERTIES(vtkCocoaTkUtilities WRAP_EXCLUDE)
    IF(VTK_REQUIRED_OBJCXX_FLAGS)
      SET_SOURCE_FILES_PROPERTIES(vtkCocoaTkUtilities
        PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
    ENDIF(VTK_REQUIRED_OBJCXX_FLAGS)
  ENDIF(VTK_USE_COCOA)

  # Use special interactor for X and Tk.
  IF(VTK_USE_X)
    SET(KitTCL_SRCS ${KitTCL_SRCS} vtkXRenderWindowTclInteractor.cxx)
    SET(KIT_TCL_LIBS ${KIT_TCL_LIBS} ${X11_LIBRARIES})
  ENDIF(VTK_USE_X)
  IF(VTK_USE_CARBON)
    SET(KIT_TCL_LIBS ${KIT_TCL_LIBS} "-framework Carbon")
  ENDIF(VTK_USE_CARBON)
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

  INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkTestingObjectFactory.cmake
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR_CM24}/CMake
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

IF (VTK_WRAP_PYTHON AND VTK_USE_TK AND TK_FOUND)
  SET(RenderingPythonTkWidgets_SRCS
    vtkTkWidgetsInit.cxx
    vtkTkRenderWidgetPython.cxx
    vtkTkImageViewerWidgetPython.cxx
    )
  IF (VTK_USE_COCOA)
    SET(RenderingPythonTkWidgets_SRCS ${RenderingPythonTkWidgets_SRCS}
        vtkCocoaTkUtilities.mm)
  ENDIF (VTK_USE_COCOA)

  VTK_ADD_LIBRARY(vtkRenderingPythonTkWidgets SHARED
              ${RenderingPythonTkWidgets_SRCS})
  SET(VTK_PYTHONTK_LIBRARIES ${VTK_TK_LIBRARIES})
  IF(VTK_USE_X)
    SET(VTK_PYTHONTK_LIBRARIES ${VTK_PYTHONTK_LIBRARIES} ${X11_LIBRARIES})
  ENDIF(VTK_USE_X)
  IF(VTK_USE_CARBON)
    SET(VTK_PYTHONTK_LIBRARIES ${VTK_PYTHONTK_LIBRARIES} "-framework Carbon")
  ENDIF(VTK_USE_CARBON)

  TARGET_LINK_LIBRARIES (vtkRenderingPythonTkWidgets
                        vtk${KIT}
                        ${VTK_PYTHONTK_LIBRARIES})

  # Apply user-defined properties to the library target.
  IF(VTK_LIBRARY_PROPERTIES)
    SET_TARGET_PROPERTIES(vtkRenderingPythonTkWidgets PROPERTIES
      ${VTK_LIBRARY_PROPERTIES})
  ENDIF(VTK_LIBRARY_PROPERTIES)

  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS vtkRenderingPythonTkWidgets
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
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
  SET(__inst_files
    ${VTK_SOURCE_DIR}/${KIT}/Testing/Cxx/vtkRegressionTestImage.h
    ${VTK_BINARY_DIR}/Rendering/vtkTDxConfigure.h
    ${VTK_SOURCE_DIR}/${KIT}/vtkTDxMotionEventInfo.h
    )
  INSTALL(FILES ${__inst_files}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

IF(NOT CMAKE_CROSSCOMPILING)
  # Add target dependency to ensure the vtkEncodeString is built before
  # Rendering is.
  ADD_DEPENDENCIES(vtk${KIT} vtkEncodeString)
ENDIF(NOT CMAKE_CROSSCOMPILING)
