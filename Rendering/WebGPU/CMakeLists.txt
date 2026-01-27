if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(VTK_USE_DAWN_WEBGPU OFF)
else()
  set(VTK_USE_DAWN_WEBGPU ON)
endif()

set(classes
    vtkWebGPUActor
    vtkWebGPUBatchedPolyDataMapper
    vtkWebGPUCamera
    vtkWebGPUCellToPrimitiveConverter
    vtkWebGPUCommandEncoderDebugGroup
    vtkWebGPUCompositePolyDataMapperDelegator
    vtkWebGPUComputeBuffer
    vtkWebGPUComputeFrustumCuller
    vtkWebGPUComputeOcclusionCuller
    vtkWebGPUComputePass
    vtkWebGPUComputePipeline
    vtkWebGPUComputePointCloudMapper
    vtkWebGPUComputeRenderBuffer
    vtkWebGPUComputeRenderTexture
    vtkWebGPUComputeTexture
    vtkWebGPUComputeTextureView
    vtkWebGPUConfiguration
    vtkWebGPUGlyph3DMapper
    vtkWebGPUHardwareSelector
    vtkWebGPUHelpers
    vtkWebGPULight
    vtkWebGPUPolyDataMapper
    vtkWebGPUPolyDataMapper2D
    vtkWebGPUProperty
    vtkWebGPURenderPipelineCache
    vtkWebGPURenderTextureCache
    vtkWebGPURenderTextureDeviceResource
    vtkWebGPURenderWindow
    vtkWebGPURenderer
    vtkWebGPUShaderDatabase
    vtkWebGPUShaderProperty
    vtkWebGPUTexture
    vtkWebGPUTextureDeviceResource
    vtkWebGPUTextureView)

set(private_headers
    "Private/vtkWebGPUActorInternals.h"
    "Private/vtkWebGPUBindGroupInternals.h"
    "Private/vtkWebGPUBindGroupLayoutInternals.h"
    "Private/vtkWebGPUBufferInternals.h"
    "Private/vtkWebGPUComputeBufferInternals.h"
    "Private/vtkWebGPUComputePassInternals.h"
    "Private/vtkWebGPUComputePassTextureStorageInternals.h"
    "Private/vtkWebGPUComputePassBufferStorageInternals.h"
    "Private/vtkWebGPUConfigurationInternals.h"
    "Private/vtkWebGPUPipelineLayoutInternals.h"
    "Private/vtkWebGPUPointCloudMapperInternals.h"
    "Private/vtkWebGPUPolyDataMapper2DInternals.h"
    "Private/vtkWebGPURenderPassCreateInfoInternals.h"
    "Private/vtkWebGPURenderPassDescriptorInternals.h"
    "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
    "Private/vtkWebGPUShaderModuleInternals.h"
    "Private/vtkWebGPUTextureInternals.h")

set(private_sources
    "Private/vtkWebGPUBindGroupInternals.cxx"
    "Private/vtkWebGPUBindGroupLayoutInternals.cxx"
    "Private/vtkWebGPUBufferInternals.cxx"
    "Private/vtkWebGPUComputeBufferInternals.cxx"
    "Private/vtkWebGPUComputePassInternals.cxx"
    "Private/vtkWebGPUComputePassTextureStorageInternals.cxx"
    "Private/vtkWebGPUComputePassBufferStorageInternals.cxx"
    "Private/vtkWebGPUConfigurationInternals.cxx"
    "Private/vtkWebGPUPipelineLayoutInternals.cxx"
    "Private/vtkWebGPUPointCloudMapperInternals.cxx"
    "Private/vtkWebGPUPolyDataMapper2DInternals.cxx"
    "Private/vtkWebGPURenderPassCreateInfoInternals.cxx"
    "Private/vtkWebGPURenderPassDescriptorInternals.cxx"
    "Private/vtkWebGPURenderPipelineDescriptorInternals.cxx"
    "Private/vtkWebGPUShaderModuleInternals.cxx"
    "Private/vtkWebGPUTextureInternals.cxx")

# setup factory overrides NI - Not Implemented
set(webgpu_overrides
    Actor
    # NI: BillboardTextActor3D
    Camera
    CompositePolyDataMapperDelegator
    # NI: LabeledContourMapper
    HardwareSelector
    # NI: ImageMapper NI: ImageSliceMapper
    Glyph3DMapper
    # NI: HyperTreeGridMapper
    Light
    # NI: PointGaussianMapper
    PolyDataMapper
    PolyDataMapper2D
    Property
    ShaderProperty
    # NI: Uniforms
    Renderer
    # NI: RenderTimerLog NI: Skybox NI: TextActor NI: TextActor3D NI: TextMapper
    Texture)

unset(wgsl_shader_sources)
unset(private_wgsl_shader_headers)
unset(public_wgsl_shader_headers)
set(private_shader_files
    wgsl/ActorColorOptions.wgsl
    wgsl/ActorRenderOptions.wgsl
    wgsl/ActorTransform.wgsl
    wgsl/CopyDepthTextureToBuffer.wgsl
    wgsl/FrustumCullingShader.wgsl
    wgsl/OcclusionCullingCopyDepthTexture.wgsl
    wgsl/OcclusionCullingDepthMipmaps.wgsl
    wgsl/OcclusionCullingPropsCulling.wgsl
    wgsl/PointCloudMapperCopyDepthFromWindow.wgsl
    wgsl/PointCloudMapperCopyDepthToWindow.wgsl
    wgsl/PointCloudMapperShader.wgsl
    wgsl/SceneLight.wgsl
    wgsl/SceneTransform.wgsl
    wgsl/Utilities.wgsl
    wgsl/VTKCellToGraphicsPrimitive.wgsl
    wgsl/vtkPolyData2DVSWGSL.wgsl
    wgsl/vtkPolyData2DFSWGSL.wgsl
    wgsl/vtkPolyDataVSWGSL.wgsl
    wgsl/vtkPolyDataFSWGSL.wgsl)

set(public_shader_files
    Testing/Cxx/TestShaders/TestComputePassShader.wgsl
    Testing/Cxx/TestShaders/TestComputePassChainedShader.wgsl
    Testing/Cxx/TestShaders/TestComputePassUniformShader.wgsl
    Testing/Cxx/TestShaders/TestComputeTextureShader.wgsl
    Testing/Cxx/TestShaders/TestComputeModifyPointColorsShader.wgsl
    Testing/Cxx/TestShaders/TestComputeModifyPointPositionsShader.wgsl
    Testing/Cxx/TestShaders/TestComputeModifyCellColorsShader.wgsl)

foreach(file IN LISTS private_shader_files)
  vtk_encode_string(
    INPUT
    "${file}"
    EXPORT_SYMBOL
    "VTKRENDERINGWEBGPU_EXPORT"
    EXPORT_HEADER
    "vtkRenderingWebGPUModule.h"
    HEADER_OUTPUT
    header
    SOURCE_OUTPUT
    source)
  list(APPEND wgsl_shader_sources "${source}")
  list(APPEND private_wgsl_shader_headers "${header}")
endforeach()

foreach(file IN LISTS public_shader_files)
  vtk_encode_string(
    INPUT
    "${file}"
    EXPORT_SYMBOL
    "VTKRENDERINGWEBGPU_EXPORT"
    EXPORT_HEADER
    "vtkRenderingWebGPUModule.h"
    HEADER_OUTPUT
    header
    SOURCE_OUTPUT
    source
    ABI_MANGLE_HEADER
    "vtkABINamespace.h"
    ABI_MANGLE_SYMBOL_BEGIN
    "VTK_ABI_NAMESPACE_BEGIN"
    ABI_MANGLE_SYMBOL_END
    "VTK_ABI_NAMESPACE_END")
  list(APPEND wgsl_shader_sources "${source}")
  list(APPEND public_wgsl_shader_headers "${header}")
endforeach()

foreach(webgpu_override IN LISTS webgpu_overrides)
  vtk_object_factory_declare(
    BASE
    "vtk${webgpu_override}"
    OVERRIDE
    "vtkWebGPU${webgpu_override}")
endforeach()

set(generated_headers vtk_wgpu.h)

set(headers)

foreach(generated_header IN LISTS generated_headers)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${generated_header}.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/${generated_header}" @ONLY)
  list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/${generated_header}")
endforeach()

if((CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
   AND (VTK_USE_X
        OR VTK_USE_Wayland
        OR VTK_USE_COCOA
        OR VTK_USE_WIN32
       ))
  message(FATAL_ERROR "VTK_USE_{X,Wayland,COCOA,WIN32} is not supported with emscripten.")
endif()
set(has_vtkRenderWindow_override 0)
set(vtk_webgpu_factory_override_classname "")
if(NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
  set(vtk_webgpu_factory_override_classname "vtkWebGPURenderWindow")
  set(has_vtkRenderWindow_override 1)
endif()

if(NOT has_vtkRenderWindow_override)
  message(
    FATAL_ERROR
      "A render window factory override was not found!"
      "This module currently supports rendering on X11, Wayland, macOS, Windows and"
      "Emscripten platforms."
      "However, you can still execute webgpu compute pipelines.")
endif()

list(APPEND classes ${vtk_webgpu_factory_override_classname})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vtkWebGPUObjectFactoryInit.cxx.in"
               "${CMAKE_CURRENT_BINARY_DIR}/vtkWebGPUObjectFactoryInit.cxx")

if(has_vtkRenderWindow_override)
  vtk_object_factory_declare(
    BASE
    vtkRenderWindow
    OVERRIDE
    ${vtk_webgpu_factory_override_classname})
endif()
vtk_object_factory_configure(
  SOURCE_FILE
  vtk_object_factory_source
  HEADER_FILE
  vtk_object_factory_header
  EXPORT_MACRO
  "VTKRENDERINGWEBGPU_EXPORT"
  EXTRA_INCLUDES
  "<vtkCollection.h>"
  "<vtkCallbackCommand.h>"
  "<vtkNew.h>"
  "<vtkObjectFactoryCollection.h>"
  "<iostream>"
  "<cstdlib>"
  INITIAL_CODE_FILE
  "${CMAKE_CURRENT_BINARY_DIR}/vtkWebGPUObjectFactoryInit.cxx")

vtk_module_add_module(
  VTK::RenderingWebGPU
  CLASSES
  ${classes}
  SOURCES
  ${vtk_object_factory_source}
  ${wgsl_shader_sources}
  ${private_sources}
  HEADERS
  ${headers}
  NOWRAP_HEADERS
  ${public_wgsl_shader_headers}
  PRIVATE_HEADERS
  ${vtk_object_factory_header}
  ${vtk_wgpu_private_headers}
  ${private_wgsl_shader_headers}
  ${private_headers})

vtk_module_compile_features(VTK::RenderingWebGPU PUBLIC cxx_std_17)

vtk_module_compile_features(VTK::RenderingWebGPU PUBLIC cxx_std_17)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_compile_options(VTK::RenderingWebGPU PUBLIC "--use-port=emdawnwebgpu")
  vtk_module_link_options(VTK::RenderingWebGPU PUBLIC "--use-port=emdawnwebgpu")
else()
  if(VTK_USE_X)
    vtk_module_find_package(PACKAGE X11)
    vtk_module_link(VTK::RenderingWebGPU PUBLIC X11::X11)
    if(TARGET X11::Xcursor)
      vtk_module_link(VTK::RenderingWebGPU PRIVATE X11::Xcursor)
    else()
      message(WARNING "X11::Xcursor not found; custom cursors will be ignored.")
    endif()
    set_property(SOURCE vtkWebGPURenderWindow.cxx APPEND PROPERTY COMPILE_DEFINITIONS "VTK_USE_X=1")
  elseif(VTK_USE_Wayland)
    vtk_module_find_package(PACKAGE WAYLAND)
    vtk_module_link(VTK::RenderingWebGPU PUBLIC WAYLAND::Client)
    set_property(SOURCE vtkWebGPURenderWindow.cxx APPEND PROPERTY COMPILE_DEFINITIONS
                                                                  "VTK_USE_Wayland=1")
  elseif(VTK_USE_COCOA)
    # TODO
  elseif(VTK_USE_WIN32_OPENGL) # FIXME: We need a VTK_USE_WIN32_WINDOW flag here
    # TODO
  endif()
  find_package(Threads REQUIRED)
  vtk_module_find_package(PACKAGE Dawn)
  vtk_module_link(VTK::RenderingWebGPU PUBLIC dawn::webgpu_dawn)
endif()
