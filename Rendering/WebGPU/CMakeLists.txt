if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(VTK_USE_DAWN_WEBGPU OFF)
else ()
  set(VTK_USE_DAWN_WEBGPU ON)
endif ()

set(classes
  vtkWebGPUActor
  vtkWebGPUCamera
  vtkWebGPUClearPass
  vtkWebGPUHardwareSelector
  vtkWebGPULight
  vtkWebGPURenderWindow
  vtkSDL2WebGPURenderWindow
  vtkWebGPUPolyDataMapper
  vtkWebGPUProperty
  vtkWebGPURenderer
  vtkWebGPURenderPass)

set(private_classes
  vtkWebGPUInternalsBindGroup
  vtkWebGPUInternalsBindGroupLayout
  vtkWebGPUInternalsBuffer
  vtkWebGPUInternalsPipelineLayout
  vtkWebGPUInternalsRenderPassCreateInfo
  vtkWebGPUInternalsRenderPassDescriptor
  vtkWebGPUInternalsRenderPipelineDescriptor
  vtkWebGPUInternalsShaderModule)

# setup factory overrides
# NI - Not Implemented
set(webgpu_overrides
  Actor
  # NI: BillboardTextActor3D
  Camera
  # NI: LabeledContourMapper
  HardwareSelector
  # NI: ImageMapper
  # NI: ImageSliceMapper
  # NI: Glyph3DMapper
  # NI: HyperTreeGridMapper
  Light
  # NI: PointGaussianMapper
  PolyDataMapper
  # NI: PolyDataMapper2D
  Property
  # NI: ShaderProperty
  # NI: Uniforms
  Renderer
  # NI: RenderTimerLog
  # NI: Skybox
  # NI: TextActor
  # NI: TextActor3D
  # NI: TextMapper
  # NI: Texture
)

unset(wgsl_shader_sources)
unset(wgsl_shader_headers)
set(shader_files
  wgsl/PolyData.wgsl)
foreach (file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGWEBGPU_NO_EXPORT"
    EXPORT_HEADER "vtkRenderingWebGPUModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND wgsl_shader_sources
    "${source}")
  list(APPEND wgsl_shader_headers
    "${header}")
endforeach ()

foreach (webgpu_override IN LISTS webgpu_overrides)
  vtk_object_factory_declare(
    BASE "vtk${webgpu_override}"
    OVERRIDE "vtkWebGPU${webgpu_override}")
endforeach ()

vtk_object_factory_declare(
  BASE vtkRenderWindow
  OVERRIDE vtkSDL2WebGPURenderWindow)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGWEBGPU_EXPORT"
  EXTRA_INCLUDES "<vtkCollection.h>" "<vtkObjectFactoryCollection.h>" "<vtkLogger.h>" "<cstdlib>"
  INITIAL_CODE_FILE "vtkWebGPUObjectFactoryInit.cxx")

set(headers
  vtkWGPUContext.h)

set(generated_headers
  vtk_wgpu.h)

foreach (generated_header IN LISTS generated_headers)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${generated_header}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${generated_header}"
    @ONLY)
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/${generated_header}")
endforeach ()

set(sources
  vtkWGPUContext.cxx)

vtk_module_add_module(VTK::RenderingWebGPU
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  SOURCES ${vtk_object_factory_source} ${sources} ${wgsl_shader_sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${vtk_wgpu_private_headers} ${wgsl_shader_headers})

vtk_module_compile_features(VTK::RenderingWebGPU
  PUBLIC
    cxx_std_14)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_include(VTK::RenderingWebGPU
    PUBLIC
      "${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include/webgpu")
  vtk_module_compile_options(VTK::RenderingWebGPU
    PUBLIC
      "-sUSE_SDL=2")
  vtk_module_link_options(VTK::RenderingWebGPU
    PUBLIC
      "-sUSE_WEBGPU=1"
      "-sUSE_SDL=2")
else()
  # WebGPU native
  if (WIN32)
    vtk_module_definitions(VTK::RenderingWebGPU PRIVATE
      "VTK_DAWN_ENABLE_BACKEND_D3D12"
    )
  elseif(ANDROID)
    vtk_module_definitions(VTK::RenderingWebGPU PRIVATE
      "VTK_DAWN_ENABLE_BACKEND_VULKAN"
    )
  elseif(UNIX)
    vtk_module_definitions(VTK::RenderingWebGPU PRIVATE
      "VTK_DAWN_ENABLE_BACKEND_VULKAN"
      "VTK_DAWN_USE_X11"
    )
  elseif(APPLE)
    vtk_module_definitions(VTK::RenderingWebGPU PRIVATE
      "VTK_DAWN_ENABLE_BACKEND_METAL"
    )
    vtk_module_link(VTK::RenderingWebGPU PRIVATE "-framework Metal")
    vtk_module_sources(VTK::RenderingWebGPU PRIVATE
      "utils_metal.mm")
  endif ()
  vtk_module_find_package(PRIVATE_IF_SHARED
    PACKAGE SDL2)
  vtk_module_find_package(PACKAGE Dawn)
  # Must include these source files here.
  vtk_module_sources(VTK::RenderingWebGPU PRIVATE
    "${DAWN_GEN_SRC_DIR}/dawn/webgpu_cpp.cpp"
    "${DAWN_GEN_SRC_DIR}/dawn/dawn_proc.c")
  vtk_module_link(VTK::RenderingWebGPU
    PRIVATE
      SDL2::SDL2
      ${DAWN_LIBRARIES})
  vtk_module_include(VTK::RenderingWebGPU
    PUBLIC
      ${DAWN_INCLUDE_DIRS})
endif ()
