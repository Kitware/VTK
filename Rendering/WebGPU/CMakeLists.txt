if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(VTK_USE_DAWN_WEBGPU OFF)
else ()
  set(VTK_USE_DAWN_WEBGPU ON)
endif ()

set(classes
  vtkWebGPUActor
  vtkWebGPUCamera
  vtkWebGPUClearPass
  vtkWebGPUComputePipeline
  vtkWebGPUComputeBuffer
  vtkWebGPUComputeRenderBuffer
  vtkWebGPUHardwareSelector
  vtkWebGPULight
  vtkWebGPURenderWindow
  vtkWebGPUPolyDataMapper
  vtkWebGPUProperty
  vtkWebGPURenderer
  vtkWebGPURenderPass)

set(private_classes
  vtkWebGPUInternalsBindGroup
  vtkWebGPUInternalsBindGroupLayout
  vtkWebGPUInternalsBuffer
  vtkWebGPUInternalsCallbacks
  vtkWebGPUInternalsComputeBuffer
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
unset(private_wgsl_shader_headers)
unset(public_wgsl_shader_headers)
set(private_shader_files
  wgsl/PolyData.wgsl)

set(public_shader_files
  Testing/Cxx/TestShaders/TestComputePipelineShader.wgsl
  Testing/Cxx/TestShaders/TestComputePipelineUniformShader.wgsl
  Testing/Cxx/TestShaders/TestComputeModifyPointColorsShader.wgsl
  Testing/Cxx/TestShaders/TestComputeModifyPointPositionsShader.wgsl
  Testing/Cxx/TestShaders/TestComputeModifyCellColorsShader.wgsl)

foreach (file IN LISTS private_shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGWEBGPU_NO_EXPORT"
    EXPORT_HEADER "vtkRenderingWebGPUModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND wgsl_shader_sources
    "${source}")
  list(APPEND private_wgsl_shader_headers
    "${header}")
endforeach ()

foreach(file IN LISTS public_shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGWEBGPU_EXPORT"
    EXPORT_HEADER "vtkRenderingWebGPUModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    ABI_MANGLE_HEADER       "vtkABINamespace.h"
    ABI_MANGLE_SYMBOL_BEGIN "VTK_ABI_NAMESPACE_BEGIN"
    ABI_MANGLE_SYMBOL_END   "VTK_ABI_NAMESPACE_END")
  list(APPEND wgsl_shader_sources
    "${source}")
  list(APPEND public_wgsl_shader_headers
    "${header}")
endforeach()

foreach (webgpu_override IN LISTS webgpu_overrides)
  vtk_object_factory_declare(
    BASE "vtk${webgpu_override}"
    OVERRIDE "vtkWebGPU${webgpu_override}")
endforeach ()

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

# Setup factory overrides and add window system specific render windows.
# FIXME: I've only brought the X render window from https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10239/commits.
# Bring in cocoa and win32 overrides as well.
# NOTE:  Supports only one override in a build. Multiple render window overrides are confusing.
if ((CMAKE_SYSTEM_NAME STREQUAL "Emscripten") AND (VTK_USE_X OR VTK_USE_COCOA OR VTK_USE_WIN32_OPENGL))
  message(FATAL_ERROR "VTK_USE_{X,COCOA,WIN32_OPENGL} is not supported with emscripten.")
endif ()
set(has_vtkRenderWindow_override 0)
set(vtk_webgpu_factory_override_classname "")
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # VTK_DEPRECATED_IN_9_4_0() Remove when vtkSDL2WebGPURenderWindow is removed.
  if (VTK_USE_SDL2)
    set(vtk_webgpu_factory_override_classname "vtkSDL2WebGPURenderWindow")
    set(has_vtkRenderWindow_override 1)
  else ()
    set(vtk_webgpu_factory_override_classname "vtkWebAssemblyWebGPURenderWindow")
    set(has_vtkRenderWindow_override 1)
  endif ()
elseif (VTK_USE_X)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    set(vtk_webgpu_factory_override_classname "vtkXWebGPURenderWindow")
    set(has_vtkRenderWindow_override 1)
  endif ()
elseif (VTK_USE_COCOA)
  # TODO: Implement
  # set(vtk_webgpu_factory_override_classname "vtkCocoaWebGPURenderWindow")
  # set(has_vtkRenderWindow_override 1)
elseif (VTK_USE_WIN32_OPENGL) # FIXME: We need a VTK_USE_WIN32_WINDOW flag here
  # TODO: Implement
  # set(vtk_webgpu_factory_override_classname "vtkWin32WebGPURenderWindow")
  # set(has_vtkRenderWindow_override 1)
endif ()

if (NOT has_vtkRenderWindow_override)
  message(FATAL_ERROR "A render window factory override was not found! This module currently supports X11 and Emscripten platforms")
endif ()

list (APPEND classes ${vtk_webgpu_factory_override_classname})
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkWebGPUObjectFactoryInit.cxx.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkWebGPUObjectFactoryInit.cxx")

vtk_object_factory_declare(
  BASE vtkRenderWindow
  OVERRIDE ${vtk_webgpu_factory_override_classname})
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGWEBGPU_EXPORT"
  EXTRA_INCLUDES "<vtkCollection.h>" "<vtkObjectFactoryCollection.h>" "<vtkLogger.h>" "<cstdlib>"
  INITIAL_CODE_FILE "${CMAKE_CURRENT_BINARY_DIR}/vtkWebGPUObjectFactoryInit.cxx")

vtk_module_add_module(VTK::RenderingWebGPU
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  SOURCES ${vtk_object_factory_source} ${sources} ${wgsl_shader_sources}
  HEADERS ${headers}
  NOWRAP_HEADERS ${public_wgsl_shader_headers}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${vtk_wgpu_private_headers} ${private_wgsl_shader_headers})

vtk_module_compile_features(VTK::RenderingWebGPU
  PUBLIC
    cxx_std_14)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_link_options(VTK::RenderingWebGPU
    PUBLIC
      "-sUSE_WEBGPU=1")
  # VTK_DEPRECATED_IN_9_4_0() Remove when vtkSDL2WebGPURenderWindow is removed.
  if (VTK_USE_SDL2)
    vtk_module_compile_options(VTK::RenderingWebGPU
      PUBLIC
        "-sUSE_SDL=2")
    vtk_module_link_options(VTK::RenderingWebGPU
      PUBLIC
        "-sUSE_SDL=2")
  endif ()
else ()
  if (VTK_USE_X)
    vtk_module_find_package(PACKAGE X11)
    vtk_module_link(VTK::RenderingWebGPU PUBLIC X11::X11)
    if (TARGET X11::Xcursor)
      vtk_module_link(VTK::RenderingWebGPU PRIVATE X11::Xcursor)
    else()
      message(WARNING "X11::Xcursor not found; custom cursors will be ignored.")
    endif()
    set_property(SOURCE vtkXOpenGLRenderWindow.cxx APPEND
      PROPERTY
        COMPILE_DEFINITIONS "VTK_HAVE_XCURSOR=$<TARGET_EXISTS:X11::Xcursor>"
    )
  elseif (VTK_USE_COCOA)
    # TODO
  elseif (VTK_USE_WIN32_OPENGL) # FIXME: We need a VTK_USE_WIN32_WINDOW flag here
    # TODO
  endif ()
  vtk_module_find_package(PACKAGE Dawn)
  # Must include these source files here.
  vtk_module_sources(VTK::RenderingWebGPU PRIVATE
    "${DAWN_GEN_SRC_DIR}/dawn/webgpu_cpp.cpp"
    "${DAWN_GEN_SRC_DIR}/dawn/dawn_proc.c")
  vtk_module_link(VTK::RenderingWebGPU
    PRIVATE
      ${DAWN_LIBRARIES})
  vtk_module_include(VTK::RenderingWebGPU
    PUBLIC
      ${DAWN_INCLUDE_DIRS})
endif ()
