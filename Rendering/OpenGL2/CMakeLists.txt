include(vtkOpenGL)
include(vtkObjectFactory)


# The TDx support has not been moved, this header is here to define nothing,
# and so allow classes in other modules to safely include it and keep going.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTDxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h)

set(Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  vtkCameraPass.cxx
  vtkClearRGBPass.cxx
  vtkClearZPass.cxx
  vtkCompositePolyDataMapper2.cxx
  vtkDefaultPass.cxx
  vtkDepthOfFieldPass.cxx
  vtkDepthImageProcessingPass.cxx
  vtkDepthPeelingPass.cxx
  vtkDualDepthPeelingPass.cxx
  vtkEDLShading.cxx
  vtkFramebufferPass.cxx
  vtkGaussianBlurPass.cxx
  vtkGenericOpenGLRenderWindow.cxx
  vtkHiddenLineRemovalPass.cxx
  vtkImageProcessingPass.cxx
  vtkLightingMapPass.cxx
  vtkLightsPass.cxx
  vtkOpaquePass.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLBillboardTextActor3D.cxx
  vtkOpenGLBufferObject.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLFXAAFilter.cxx
  vtkOpenGLFramebufferObject.cxx
  vtkOpenGLGL2PSHelper.cxx
  vtkOpenGLGlyph3DHelper.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSelector.cxx
  vtkOpenGLHelper.cxx
  vtkOpenGLImageAlgorithmHelper.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLIndexBufferObject.cxx
  vtkOpenGLInstanceCulling.cxx
  vtkOpenGLLabeledContourMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPointGaussianMapper.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLQuadHelper.cxx
  vtkOpenGLRenderPass.cxx
  vtkOpenGLRenderTimer.cxx
  vtkOpenGLRenderTimerLog.cxx
  vtkOpenGLRenderUtilities.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLShaderCache.cxx
  vtkOpenGLSkybox.cxx
  vtkOpenGLSphereMapper.cxx
  vtkOpenGLState.cxx
  vtkOpenGLStickMapper.cxx
  vtkOpenGLTextActor.cxx
  vtkOpenGLTextActor3D.cxx
  vtkOpenGLTextMapper.cxx
  vtkOpenGLTexture.cxx
  vtkOpenGLVertexArrayObject.cxx
  vtkOpenGLVertexBufferObject.cxx
  vtkOpenGLVertexBufferObjectCache.cxx
  vtkOpenGLVertexBufferObjectGroup.cxx
  vtkOrderIndependentTranslucentPass.cxx
  vtkOverlayPass.cxx
  vtkPixelBufferObject.cxx
  vtkPointFillPass.cxx
  vtkRenderPassCollection.cxx
  vtkRenderStepsPass.cxx
  vtkRenderbuffer.cxx
  vtkSSAAPass.cxx
  vtkSequencePass.cxx
  vtkShader.cxx
  vtkShaderProgram.cxx
  vtkShadowMapBakerPass.cxx
  vtkShadowMapPass.cxx
  vtkSimpleMotionBlurPass.cxx
  vtkSobelGradientMagnitudePass.cxx
  vtkTextureObject.cxx
  vtkTextureUnitManager.cxx
  vtkToneMappingPass.cxx
  vtkTransformFeedback.cxx
  vtkTranslucentPass.cxx
  vtkValuePass.cxx
  vtkVolumetricPass.cxx
  )

if (NOT DEFINED OPENGL_ES_VERSION)
  set(Module_SRCS ${Module_SRCS}
    vtkDataTransferHelper.cxx
    )
endif()

set(${vtk-module}_HDRS
  vtkOpenGL.h
  vtkStateStorage.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h
  )

set(shader_files
  glsl/vtkDepthOfFieldPassFS.glsl
  glsl/vtkDepthPeelingPassFinalFS.glsl
  glsl/vtkDepthPeelingPassIntermediateFS.glsl
  glsl/vtkEDLBilateralFilterFS.glsl
  glsl/vtkEDLComposeFS.glsl
  glsl/vtkEDLShadeFS.glsl
  glsl/vtkFXAAFilterFS.glsl
  glsl/vtkGaussianBlurPassFS.glsl
  glsl/vtkGaussianBlurPassVS.glsl
  glsl/vtkGlyph3DVS.glsl
  glsl/vtkOrderIndependentTranslucentPassFinalFS.glsl
  glsl/vtkPointGaussianVS.glsl
  glsl/vtkPointGaussianGS.glsl
  glsl/vtkPointFillPassFS.glsl
  glsl/vtkPolyData2DFS.glsl
  glsl/vtkPolyData2DVS.glsl
  glsl/vtkPolyDataFS.glsl
  glsl/vtkPolyDataVS.glsl
  glsl/vtkPolyDataWideLineGS.glsl
  glsl/vtkSSAAPassFS.glsl
  glsl/vtkSimpleMotionBlurPassFS.glsl
  glsl/vtkSobelGradientMagnitudePass1FS.glsl
  glsl/vtkSobelGradientMagnitudePass2FS.glsl
  glsl/vtkSphereMapperGS.glsl
  glsl/vtkStickMapperGS.glsl
  glsl/vtkTextureObjectFS.glsl
  glsl/vtkTextureObjectVS.glsl
  )
unset(shader_h_files)
foreach(file ${shader_files})
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGOPENGL2_EXPORT"
    EXPORT_HEADER "vtkRenderingOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
endforeach()

if(VTK_USE_X)
  find_package(X11 REQUIRED)
  if(NOT X11_Xt_FOUND)
    message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
  endif()
  include_directories(${X11_INCLUDE_DIR} ${X11_Xt_INCLUDE_PATH})
endif()

# setup factory overrides
set(opengl_overrides
  Actor
  BillboardTextActor3D
  Camera
  LabeledContourMapper
  HardwareSelector
  ImageMapper
  ImageSliceMapper
  Glyph3DMapper
  Light
  PointGaussianMapper
  PolyDataMapper
  PolyDataMapper2D
  Property
  Renderer
  RenderTimerLog
  Skybox
  TextActor
  TextActor3D
  TextMapper
  Texture
  )

# Now to see about the GPU info list.
list(APPEND vtk_module_overrides "vtkGPUInfoList")
list(APPEND Module_SRCS "vtkDummyGPUInfoList.cxx")
set(vtk_module_vtkGPUInfoList_override "vtkDummyGPUInfoList")

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()


# Update sources to include render window specializations for
# offscreen-only modes, if enabled.
if(VTK_OPENGL_HAS_OSMESA)
  list(APPEND Module_SRCS vtkOSOpenGLRenderWindow.cxx)
endif()
if(VTK_OPENGL_HAS_EGL OR ANDROID)
  list(APPEND Module_SRCS vtkEGLRenderWindow.cxx)
  set_source_files_properties(vtkEGLRenderWindow.cxx
    PROPERTIES COMPILE_DEFINITIONS
    "VTK_DEFAULT_EGL_DEVICE_INDEX=${VTK_DEFAULT_EGL_DEVICE_INDEX}")
endif()

# Setup factory overrides and add window system specific render windows.
if(VTK_USE_X)
  # Add some custom overrides
  if(NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_add_override("vtkRenderWindowInteractor" "vtkXRenderWindowInteractor")
    vtk_add_override("vtkRenderWindow" "vtkXOpenGLRenderWindow")
  endif()
  list(APPEND Module_SRCS
    vtkXRenderWindowInteractor.cxx
    vtkXOpenGLRenderWindow.cxx)
elseif(WIN32)
  # Add some custom overrides
  if(NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_add_override("vtkRenderWindowInteractor" "vtkWin32RenderWindowInteractor")
    vtk_add_override("vtkRenderWindow" "vtkWin32OpenGLRenderWindow")
  endif()
  list(APPEND Module_SRCS
    vtkWin32RenderWindowInteractor.cxx
    vtkWin32OpenGLRenderWindow.cxx
    )
elseif(VTK_USE_COCOA)
  # Add some custom overrides
  if(NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_add_override("vtkRenderWindowInteractor" "vtkCocoaRenderWindowInteractor")
    vtk_add_override("vtkRenderWindow" "vtkCocoaRenderWindow")
  endif()
  list(APPEND Module_SRCS
    vtkCocoaRenderWindowInteractor.mm
    vtkCocoaRenderWindow.mm
    vtkCocoaGLView.mm
    )
  list(APPEND ${vtk-module}_HDRS
    vtkCocoaMacOSXSDKCompatibility.h
    )
  # Set specified Objective-C++ flags, if any.
  if(VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(vtkCocoaRenderWindowInteractor.mm
                                vtkCocoaRenderWindow.mm
                                vtkCocoaGLView.mm
                                PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif()
elseif(ANDROID)
  # Add some custom overrides
  vtk_add_override("vtkRenderWindowInteractor" "vtkAndroidRenderWindowInteractor")
  vtk_add_override("vtkRenderWindow" "vtkEGLRenderWindow")
  list(APPEND Module_SRCS
    vtkAndroidRenderWindowInteractor.cxx
    # vtkEGLRenderWindow.cxx -- already added above.
    )
  include_directories(
    "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue"
    )
elseif(APPLE_IOS)
  # Add some custom overrides
  vtk_add_override("vtkRenderWindowInteractor" "vtkIOSRenderWindowInteractor")
  vtk_add_override("vtkRenderWindow" "vtkIOSRenderWindow")
  list(APPEND Module_SRCS
    vtkIOSRenderWindowInteractor.mm
    vtkIOSRenderWindow.mm
    )
  # Set specified Objective-C++ flags, if any.
  if(VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(vtkIOSRenderWindowInteractor.mm
                                vtkIOSRenderWindow.mm
                                PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif()
endif()

# If no overrides were defined for vtkRenderWindow and any of the offscreen
# features were available, make the appropriate render window the default.
if(VTK_DEFAULT_RENDER_WINDOW_HEADLESS OR NOT DEFINED vtk_module_vtkRenderWindow_override)
  if(VTK_OPENGL_HAS_EGL)
    vtk_add_override("vtkRenderWindow" "vtkEGLRenderWindow")
  elseif(VTK_OPENGL_HAS_OSMESA)
    vtk_add_override("vtkRenderWindow" "vtkOSOpenGLRenderWindow")
  endif()
endif()

vtk_object_factory_configure("${vtk_module_overrides}")

# Detect and clear OpenGL errors. If not then other classes/user code
# may detect VTK's OpenGL errors and vice-versa.
set(VTK_REPORT_OPENGL_ERRORS ON CACHE BOOL "Enable OpenGL error check and report")
cmake_dependent_option(VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS
  "Enable OpenGL error check and reporting in non-debug builds." OFF
  VTK_REPORT_OPENGL_ERRORS OFF)
mark_as_advanced(VTK_REPORT_OPENGL_ERRORS VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGLError.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h)

# Add OpenGL command stream annotations via
# vtkOpenGLRenderUtilities::MarkDebugEvent when enabled:
option(VTK_OPENGL_ENABLE_STREAM_ANNOTATIONS
  "Enable debugging annotations in the OpenGL command stream."
  OFF
)
mark_as_advanced(VTK_OPENGL_ENABLE_STREAM_ANNOTATIONS)

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h)
# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkRenderingOpenGLConfigure.h\"")

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_opengl_link(${vtk-module})
if(VTK_USE_X)
  vtk_module_link_libraries(${vtk-module}
    LINK_PUBLIC ${X11_LIBRARIES} ${X11_Xt_LIB})
elseif(VTK_USE_COCOA)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework Cocoa")
endif()

if (WIN32 AND MSVC)
  vtk_module_link_libraries(${vtk-module} LINK_PRIVATE dbghelp)
endif()

if(ANDROID)
  target_link_libraries(${vtk-module}
    LINK_PUBLIC ${OPENGL_egl_LIBRARY}
    LINK_PRIVATE android
    )
endif()

if (APPLE_IOS)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework UIKit")
endif()

if (NOT ANDROID AND NOT APPLE_IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL
    "Emscripten" AND NOT VTK_OPENGL_USE_GLES)
  VTK_ADD_EXECUTABLE(vtkProbeOpenGLVersion vtkProbeOpenGLVersion.cxx)
  target_link_libraries(vtkProbeOpenGLVersion ${vtk-module})
endif()

if (WIN32 AND ( NOT CMAKE_CROSSCOMPILING OR DEFINED CMAKE_CROSSCOMPILING_EMULATOR) )
  VTK_ADD_EXECUTABLE(vtkTestOpenGLVersion vtkTestOpenGLVersion.cxx)
  set_target_properties(vtkTestOpenGLVersion
    PROPERTIES
      WIN32_EXECUTABLE TRUE
      )
  target_link_libraries(vtkTestOpenGLVersion opengl32)
endif()
