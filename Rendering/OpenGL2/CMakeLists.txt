set(classes
  vtkArrayRenderer
  vtkCameraPass
  vtkClearRGBPass
  vtkClearZPass
  vtkDataTransferHelper
  vtkDefaultPass
  vtkDepthImageProcessingPass
  vtkDepthOfFieldPass
  vtkDepthPeelingPass
  vtkDrawTexturedElements
  vtkDualDepthPeelingPass
  vtkEDLShading
  vtkEquirectangularToCubeMapTexture
  vtkFastLabeledDataMapper
  vtkFramebufferPass
  vtkGaussianBlurPass
  vtkGenericOpenGLRenderWindow
  vtkGLSLModCamera
  vtkGLSLModCoincidentTopology
  vtkGLSLModLight
  vtkGLSLModPixelDebugger
  vtkGLSLModifierBase
  vtkGLSLModifierFactory
  vtkHiddenLineRemovalPass
  vtkImageProcessingPass
  vtkLightingMapPass
  vtkLightsPass
  vtkOpaquePass
  vtkOpenGLActor
  vtkOpenGLArrayTextureBufferAdapter
  vtkOpenGLBatchedPolyDataMapper
  vtkOpenGLBillboardTextActor3D
  vtkOpenGLBufferObject
  vtkOpenGLCamera
  vtkOpenGLCellToVTKCellMap
  vtkOpenGLCompositePolyDataMapperDelegator
  vtkOpenGLES30PolyDataMapper2D
  vtkOpenGLFXAAFilter
  vtkOpenGLFXAAPass
  vtkOpenGLFluidMapper
  vtkOpenGLFramebufferObject
  vtkOpenGLGL2PSHelper
  vtkOpenGLGlyph3DHelper
  vtkOpenGLGlyph3DMapper
  vtkOpenGLHardwareSelector
  vtkOpenGLHelper
  vtkOpenGLHyperTreeGridMapper
  vtkOpenGLImageAlgorithmHelper
  vtkOpenGLImageMapper
  vtkOpenGLImageSliceMapper
  vtkOpenGLIndexBufferObject
  vtkOpenGLInstanceCulling
  vtkOpenGLLabeledContourMapper
  vtkOpenGLLight
  vtkOpenGLLowMemoryBatchedPolyDataMapper
  vtkOpenGLLowMemoryPolyDataMapper
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
  vtkOSOpenGLRenderWindow
  vtkOutlineGlowPass
  vtkOverlayPass
  vtkPBRIrradianceTexture
  vtkPBRLUTTexture
  vtkPBRPrefilterTexture
  vtkPanoramicProjectionPass
  vtkPixelBufferObject
  vtkPointFillPass
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

set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h")

set(nowrap_headers
  vtkOpenGLPointGaussianMapperHelper.h
  vtkOpenGLResourceFreeCallback.h	
  vtkOpenGLShaderDeclaration.h
)

# add opengl support test executable on windows
if (VTK_USE_WIN32_OPENGL AND (NOT CMAKE_CROSSCOMPILING OR DEFINED CMAKE_CROSSCOMPILING_EMULATOR))
  vtk_module_add_executable(vtkTestOpenGLVersion vtkTestOpenGLVersion.cxx)
  set_target_properties(vtkTestOpenGLVersion
    PROPERTIES
      WIN32_EXECUTABLE TRUE)
  target_link_libraries(vtkTestOpenGLVersion PRIVATE opengl32 VTK::CommonCore)
  list(APPEND headers vtkTestOpenGLVersion.h)
endif ()

set(sources)
set(private_headers)
set(private_classes
  vtkOpenGLLowMemoryCellTypeAgent
  vtkOpenGLLowMemoryLinesAgent
  vtkOpenGLLowMemoryPolygonsAgent
  vtkOpenGLLowMemoryStripsAgent
  vtkOpenGLLowMemoryVerticesAgent
)

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
  glsl/vtkOutlineGlowBlurPassFS.glsl
  glsl/vtkOutlineGlowUpscalePassFS.glsl
  glsl/vtkPBRFunctions.glsl
  glsl/vtkPointFillPassFS.glsl
  glsl/vtkPointGaussianVS.glsl
  glsl/vtkPointGaussianGS.glsl
  glsl/vtkPolyData2DFS.glsl
  glsl/vtkPolyData2DVS.glsl
  glsl/vtkPolyDataFS.glsl
  glsl/vtkPolyDataVS.glsl
  glsl/vtkPolyDataEdgesGS.glsl
  glsl/vtkPolyDataWideLineGS.glsl
  glsl/vtkSSAAPassFS.glsl
  glsl/vtkSimpleMotionBlurPassFS.glsl
  glsl/vtkSimpleSplatVS.glsl
  glsl/vtkSobelGradientMagnitudePass1FS.glsl
  glsl/vtkSobelGradientMagnitudePass2FS.glsl
  glsl/vtkSphereMapperGS.glsl
  glsl/vtkStickMapperGS.glsl
  glsl/vtkTextureObjectFS.glsl)
set(public_shader_files
  glsl/vtkTextureObjectVS.glsl)
unset(shader_h_files)
foreach(file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGOPENGL2_NO_EXPORT"
    EXPORT_HEADER "vtkRenderingOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources
    "${source}")
  list(APPEND private_headers
    "${header}")
endforeach()
foreach(file IN LISTS public_shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGOPENGL2_EXPORT"
    EXPORT_HEADER "vtkRenderingOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    ABI_MANGLE_HEADER       "vtkABINamespace.h"
    ABI_MANGLE_SYMBOL_BEGIN "VTK_ABI_NAMESPACE_BEGIN"
    ABI_MANGLE_SYMBOL_END   "VTK_ABI_NAMESPACE_END")
  list(APPEND sources
    "${source}")
  list(APPEND nowrap_headers
    "${header}")
endforeach()

vtk_encode_string(
  INPUT "textures/BlueNoiseTexture64x64.jpg"
  HEADER_OUTPUT BlueNoiseTexture64x64_header
  SOURCE_OUTPUT BlueNoiseTexture64x64_source
  EXPORT_SYMBOL "VTKRENDERINGOPENGL2_NO_EXPORT"
  EXPORT_HEADER "vtkRenderingOpenGL2Module.h"
  BINARY)
list(APPEND sources "${BlueNoiseTexture64x64_source}")
list(APPEND private_headers "${BlueNoiseTexture64x64_header}")

# setup factory overrides
set(opengl_overrides
  Actor
  CompositePolyDataMapperDelegator
  BillboardTextActor3D
  Camera
  LabeledContourMapper
  HardwareSelector
  ImageMapper
  ImageSliceMapper
  Glyph3DMapper
  HyperTreeGridMapper
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

include(vtkOpenGLOptions)
if (VTK_OPENGL_USE_GLES)
  # Add custom overrides
  vtk_object_factory_declare(
    BASE "vtkPolyDataMapper"
    OVERRIDE "vtkOpenGLLowMemoryPolyDataMapper")
  vtk_object_factory_declare(
    BASE "vtkPolyDataMapper2D"
    OVERRIDE "vtkOpenGLES30PolyDataMapper2D")
  # remove default opengl override
  list(REMOVE_ITEM opengl_overrides PolyDataMapper)
  list(REMOVE_ITEM opengl_overrides PolyDataMapper2D)
endif ()

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
set(has_vtkRenderWindow_override 0)
if (VTK_OPENGL_HAS_EGL)
  list(APPEND classes vtkEGLRenderWindow)
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkOpenGLRenderWindow)
    set(has_vtkRenderWindow_override 1)
  endif ()
  set_source_files_properties(vtkEGLRenderWindow.cxx
    PROPERTIES
      COMPILE_DEFINITIONS "VTK_DEFAULT_EGL_DEVICE_INDEX=${VTK_DEFAULT_EGL_DEVICE_INDEX}")
endif ()

# Setup factory overrides and add window system specific render windows.
# On Linux and Windows, it's possible to have multiple context creation backends that can be used
# as fallback. vtkOpenGLRenderWindow is made the factory override for vtkRenderWindow because
# vtkOpenGLRenderWindow has runtime detection of suitable backend in it's New() method.
if (VTK_USE_X)
  list(APPEND classes vtkXOpenGLRenderWindow)
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS AND NOT has_vtkRenderWindow_override)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkOpenGLRenderWindow)
    set(has_vtkRenderWindow_override 1)
  endif ()
elseif (VTK_USE_WIN32_OPENGL)
  list(APPEND classes
    vtkWin32OpenGLRenderWindow
    vtkWin32OpenGLDXRenderWindow)
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS AND NOT has_vtkRenderWindow_override)
    vtk_object_factory_declare(
      BASE vtkRenderWindow
      OVERRIDE vtkOpenGLRenderWindow)
    set(has_vtkRenderWindow_override 1)
  endif ()
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
    vtkCocoaRenderWindow.h)
  list(APPEND nowrap_headers
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
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindow
    OVERRIDE vtkWebAssemblyOpenGLRenderWindow)
  set(has_vtkRenderWindow_override 1)
  list(APPEND classes vtkWebAssemblyOpenGLRenderWindow)
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
  vtk_object_factory_declare(
    BASE vtkRenderWindow
    OVERRIDE vtkOSOpenGLRenderWindow)
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
  PRIVATE_CLASSES ${private_classes}
  SOURCES ${sources} ${vtk_object_factory_source}
  HEADERS ${headers}
  NOWRAP_HEADERS ${nowrap_headers}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header})

if (VTK_USE_X)
  vtk_module_find_package(PACKAGE X11)
  vtk_module_link(VTK::RenderingOpenGL2 PUBLIC X11::X11)
  if (TARGET X11::Xcursor)
    vtk_module_link(VTK::RenderingOpenGL2 PRIVATE X11::Xcursor)
  else()
    message(WARNING "X11::Xcursor not found; custom cursors will be ignored.")
  endif()
  set_property(SOURCE vtkXOpenGLRenderWindow.cxx APPEND
    PROPERTY
      COMPILE_DEFINITIONS "VTK_HAVE_XCURSOR=$<TARGET_EXISTS:X11::Xcursor>"
  )
elseif (VTK_USE_COCOA)
  vtk_module_link(VTK::RenderingOpenGL2 PUBLIC "-framework Cocoa")
elseif (VTK_USE_WIN32_OPENGL)
  # vtkWin32OpenGLDXRenderWindow requires D3D.
  vtk_module_find_package(PRIVATE_IF_SHARED
    PACKAGE DirectX)
  vtk_module_link(VTK::RenderingOpenGL2 NO_KIT_EXPORT_IF_SHARED PRIVATE DirectX::d3d11 DirectX::dxgi)
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
vtk_add_test_mangling(VTK::RenderingOpenGL2)
