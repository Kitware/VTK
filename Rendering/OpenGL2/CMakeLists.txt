set(classes
  vtkCameraPass
  vtkClearRGBPass
  vtkClearZPass
  vtkCompositePolyDataMapper2
  vtkDefaultPass
  vtkDepthImageProcessingPass
  vtkDepthOfFieldPass
  vtkDepthPeelingPass
  vtkDualDepthPeelingPass
  vtkEDLShading
  vtkEquirectangularToCubeMapTexture
  vtkFramebufferPass
  vtkGaussianBlurPass
  vtkGenericOpenGLRenderWindow
  vtkHiddenLineRemovalPass
  vtkImageProcessingPass
  vtkLightingMapPass
  vtkLightsPass
  vtkOpaquePass
  vtkOpenGLActor
  vtkOpenGLBillboardTextActor3D
  vtkOpenGLBufferObject
  vtkOpenGLCamera
  vtkOpenGLCellToVTKCellMap
  vtkOpenGLFXAAFilter
  vtkOpenGLFXAAPass
  vtkOpenGLFluidMapper
  vtkOpenGLFramebufferObject
  vtkOpenGLGL2PSHelper
  vtkOpenGLGlyph3DHelper
  vtkOpenGLGlyph3DMapper
  vtkOpenGLHardwareSelector
  vtkOpenGLHelper
  vtkOpenGLImageAlgorithmHelper
  vtkOpenGLImageMapper
  vtkOpenGLImageSliceMapper
  vtkOpenGLIndexBufferObject
  vtkOpenGLInstanceCulling
  vtkOpenGLLabeledContourMapper
  vtkOpenGLLight
  vtkOpenGLPointGaussianMapper
  vtkOpenGLPolyDataMapper
  vtkOpenGLPolyDataMapper2D
  vtkOpenGLProperty
  vtkOpenGLQuadHelper
  vtkOpenGLRenderPass
  vtkOpenGLRenderTimer
  vtkOpenGLRenderTimerLog
  vtkOpenGLRenderUtilities
  vtkOpenGLRenderWindow
  vtkOpenGLRenderer
  vtkOpenGLShaderCache
  vtkOpenGLShaderProperty
  vtkOpenGLSkybox
  vtkOpenGLSphereMapper
  vtkOpenGLState
  vtkOpenGLStickMapper
  vtkOpenGLTextActor
  vtkOpenGLTextActor3D
  vtkOpenGLTextMapper
  vtkOpenGLTexture
  vtkOpenGLUniforms
  vtkOpenGLVertexArrayObject
  vtkOpenGLVertexBufferObject
  vtkOpenGLVertexBufferObjectCache
  vtkOpenGLVertexBufferObjectGroup
  vtkOrderIndependentTranslucentPass
  vtkOverlayPass
  vtkPanoramicProjectionPass
  vtkPixelBufferObject
  vtkPointFillPass
  vtkPBRIrradianceTexture
  vtkPBRLUTTexture
  vtkPBRPrefilterTexture
  vtkRenderPassCollection
  vtkRenderStepsPass
  vtkRenderbuffer
  vtkSSAAPass
  vtkSSAOPass
  vtkSequencePass
  vtkShader
  vtkShaderProgram
  vtkShadowMapBakerPass
  vtkShadowMapPass
  vtkSimpleMotionBlurPass
  vtkSobelGradientMagnitudePass
  vtkTextureObject
  vtkTextureUnitManager
  vtkToneMappingPass
  vtkTransformFeedback
  vtkTranslucentPass
  vtkValuePass
  vtkVolumetricPass)

if (NOT DEFINED OPENGL_ES_VERSION)
  list(APPEND classes
    vtkDataTransferHelper)
endif()

set(headers
  vtkCompositePolyDataMapper2Internal.h
  vtkOpenGL.h
  vtkStateStorage.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h")

# add opengl support test execulatble on windows
if (WIN32 AND (NOT CMAKE_CROSSCOMPILING OR DEFINED CMAKE_CROSSCOMPILING_EMULATOR))
  vtk_module_add_executable(vtkTestOpenGLVersion vtkTestOpenGLVersion.cxx)
  set_target_properties(vtkTestOpenGLVersion
    PROPERTIES
      WIN32_EXECUTABLE TRUE)
  target_link_libraries(vtkTestOpenGLVersion PRIVATE opengl32)
  list(APPEND headers vtkTestOpenGLVersion.h)
endif ()

set(sources)
set(private_headers)

set(shader_files
  glsl/vtkDepthOfFieldPassFS.glsl
  glsl/vtkDepthPeelingPassFinalFS.glsl
  glsl/vtkDepthPeelingPassIntermediateFS.glsl
  glsl/vtkEDLBilateralFilterFS.glsl
  glsl/vtkEDLComposeFS.glsl
  glsl/vtkEDLShadeFS.glsl
  glsl/vtkFXAAFilterFS.glsl
  glsl/vtkFluidMapperDepthFilterBiGaussFS.glsl
  glsl/vtkFluidMapperDepthFilterNarrowRangeFS.glsl
  glsl/vtkFluidMapperSurfaceNormalFS.glsl
  glsl/vtkFluidMapperThicknessAndVolumeColorFilterFS.glsl
  glsl/vtkFluidMapperFS.glsl
  glsl/vtkFluidMapperFinalFS.glsl
  glsl/vtkFluidMapperGS.glsl
  glsl/vtkFluidMapperVS.glsl
  glsl/vtkGaussianBlurPassFS.glsl
  glsl/vtkGaussianBlurPassVS.glsl
  glsl/vtkGlyph3DVS.glsl
  glsl/vtkOrderIndependentTranslucentPassFinalFS.glsl
  glsl/vtkPointFillPassFS.glsl
  glsl/vtkPointGaussianVS.glsl
  glsl/vtkPointGaussianGS.glsl
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
  glsl/vtkTextureObjectVS.glsl)
unset(shader_h_files)
foreach(file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGOPENGL2_EXPORT"
    EXPORT_HEADER "vtkRenderingOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources
    "${source}")
  list(APPEND private_headers
    "${header}")
endforeach()

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
  ShaderProperty
  Uniforms
  Renderer
  RenderTimerLog
  Skybox
  TextActor
  TextActor3D
  TextMapper
  Texture)

# Now to see about the GPU info list.
list(APPEND classes vtkDummyGPUInfoList)
vtk_object_factory_declare(
  BASE vtkGPUInfoList
  OVERRIDE vtkDummyGPUInfoList)

foreach (opengl_override IN LISTS opengl_overrides)
  vtk_object_factory_declare(
    BASE "vtk${opengl_override}"
    OVERRIDE "vtkOpenGL${opengl_override}")
endforeach ()

# Update sources to include render window specializations for
# offscreen-only modes, if enabled.
if (VTK_OPENGL_HAS_OSMESA)
  list(APPEND classes vtkOSOpenGLRenderWindow)
endif ()
if (VTK_OPENGL_HAS_EGL)
  list(APPEND classes vtkEGLRenderWindow)
  set_source_files_properties(vtkEGLRenderWindow.cxx
    PROPERTIES
      COMPILE_DEFINITIONS "VTK_DEFAULT_EGL_DEVICE_INDEX=${VTK_DEFAULT_EGL_DEVICE_INDEX}")
endif ()

set(has_vtkRenderWindow_override 0)
# Setup factory overrides and add window system specific render windows.
if (VTK_USE_X)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkXOpenGLRenderWindow)
    set(has_vtkRenderWindow_override 1)
  endif ()
  list(APPEND classes vtkXOpenGLRenderWindow)
elseif (WIN32)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkWin32OpenGLRenderWindow)
    set(has_vtkRenderWindow_override 1)
  endif ()
  list(APPEND classes vtkWin32OpenGLRenderWindow)
elseif (VTK_USE_COCOA)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkCocoaRenderWindow)
    set(has_vtkRenderWindow_override 1)
  endif ()
  list(APPEND sources
    vtkCocoaRenderWindow.mm
    vtkCocoaGLView.mm)
  list(APPEND headers
    vtkCocoaRenderWindow.h
    vtkCocoaGLView.h)
  # Set specified Objective-C++ flags, if any.
  if (VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      vtkCocoaRenderWindow.mm
      vtkCocoaGLView.mm
      PROPERTIES
        COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
elseif (ANDROID)
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindow
    OVERRIDE vtkEGLRenderWindow)
  set(has_vtkRenderWindow_override 1)
  include_directories("${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
elseif (APPLE_IOS)
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindow
    OVERRIDE vtkIOSRenderWindow)
  set(has_vtkRenderWindow_override 1)
  list(APPEND sources vtkIOSRenderWindow.mm)
  list(APPEND headers vtkIOSRenderWindow.h)
  # Set specified Objective-C++ flags, if any.
  if (VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      vtkIOSRenderWindow.mm
      PROPERTIES
        COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
endif ()

if (VTK_DEFAULT_RENDER_WINDOW_HEADLESS OR NOT has_vtkRenderWindow_override)
  if (VTK_OPENGL_HAS_EGL)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkEGLRenderWindow)
  elseif (VTK_OPENGL_HAS_OSMESA)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkOSOpenGLRenderWindow)
  endif ()
endif ()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGOPENGL2_EXPORT")

# Detect and clear OpenGL errors. If not then other classes/user code
# may detect VTK's OpenGL errors and vice-versa.
set(VTK_REPORT_OPENGL_ERRORS ON CACHE BOOL "Enable OpenGL error check and report")
cmake_dependent_option(VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS
  "Enable OpenGL error check and reporting in non-debug builds." OFF
  VTK_REPORT_OPENGL_ERRORS OFF)
mark_as_advanced(VTK_REPORT_OPENGL_ERRORS)
mark_as_advanced(VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGLError.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h")

# Add OpenGL command stream annotations via
# vtkOpenGLRenderUtilities::MarkDebugEvent when enabled:
option(VTK_OPENGL_ENABLE_STREAM_ANNOTATIONS
  "Enable debugging annotations in the OpenGL command stream."
  OFF
)
mark_as_advanced(VTK_OPENGL_ENABLE_STREAM_ANNOTATIONS)

# Configure the module specific settings into a module configured header.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h")

vtk_module_add_module(VTK::RenderingOpenGL2
  CLASSES ${classes}
  SOURCES ${sources} ${vtk_object_factory_source}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header})

if (VTK_USE_X)
  vtk_module_find_package(
    PACKAGE X11)
  vtk_module_link(VTK::RenderingOpenGL2
    PUBLIC
      X11::X11)
elseif (VTK_USE_COCOA)
  vtk_module_link(VTK::RenderingOpenGL2 PUBLIC "-framework Cocoa")
endif ()

if (ANDROID)
  vtk_module_link(VTK::RenderingOpenGL2
    PRIVATE
      android)
endif ()

if (APPLE_IOS)
  vtk_module_link(VTK::RenderingOpenGL2 PUBLIC "-framework UIKit")
endif ()

if (NOT ANDROID AND
    NOT APPLE_IOS AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten" AND
    NOT VTK_OPENGL_USE_GLES)
  set(probe_no_install)
  if (NOT _vtk_build_INSTALL_HEADERS)
    set(probe_no_install
      NO_INSTALL)
  endif ()
  vtk_module_add_executable(vtkProbeOpenGLVersion
    ${probe_no_install}
    vtkProbeOpenGLVersion.cxx)
  target_link_libraries(vtkProbeOpenGLVersion
    PRIVATE
      VTK::RenderingOpenGL2)
  vtk_module_autoinit(
    TARGETS vtkProbeOpenGLVersion
    MODULES VTK::RenderingOpenGL2)
endif ()
