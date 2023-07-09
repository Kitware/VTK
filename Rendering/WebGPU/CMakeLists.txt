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
  vtkWebGPURenderPass
)

set(private_classes
  vtkWebGPUInternalsBindGroup
  vtkWebGPUInternalsBindGroupLayout
  vtkWebGPUInternalsBuffer
  vtkWebGPUInternalsPipelineLayout
  vtkWebGPUInternalsRenderPassCreateInfo
  vtkWebGPUInternalsRenderPassDescriptor
  vtkWebGPUInternalsRenderPipelineDescriptor
  vtkWebGPUInternalsShaderModule
)

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
  wgsl/PolyData.wgsl
)
foreach(file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGWEBGPU_EXPORT"
    EXPORT_HEADER "vtkRenderingWebGPUModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND wgsl_shader_sources
    "${source}")
  list(APPEND wgsl_shader_headers
    "${header}")
endforeach()

foreach (webgpu_override IN LISTS webgpu_overrides)
  vtk_object_factory_declare(
    BASE "vtk${webgpu_override}"
    OVERRIDE "vtkWebGPU${webgpu_override}")
endforeach ()

vtk_object_factory_declare(
  BASE vtkRenderWindow
  OVERRIDE vtkSDL2WebGPURenderWindow)

set (intial_cpp_code
  "
  // defaults
  int use_opengl = 1\;
  int use_webgpu = 0\;
  // Query the enviromental for backend to use.
  if(const char* p = std::getenv(\"VTK_GRAPHICS_BACKEND\"))
  {
    const std::string backend(p)\;
    if(backend == \"OPENGL\")
    {
      use_webgpu = 0\;
      use_opengl = 1\;
    }
    else if(backend == \"WEBGPU\")
    {
      #ifndef VTK_ENABLE_WEBGPU
      vtkLogF(ERROR,\"WEBGPU backend requested but the WEBGPU backend was not compiled\")\;
      #else
      use_webgpu = 1\;
      use_opengl = 0\;
      #endif
    }
    else
    {
      vtkLogF(ERROR,\"Unknown backend '%p', using default OPENGL\",backend.c_str())\;
    }
  }
    // register the factory before modyfing the overrrides so it is found below
  vtkRenderingWebGPUObjectFactory* renderingFactory = vtkRenderingWebGPUObjectFactory::New()\;
  if (renderingFactory)
  {
    // vtkObjectFactory keeps a reference to the factory,
    vtkObjectFactory::RegisterFactory(renderingFactory)\;
    renderingFactory->Delete()\;
  }
  vtkObjectFactory* object_factory\;
  vtkCollectionSimpleIterator osit\;
  for (vtkObjectFactory::GetRegisteredFactories()->InitTraversal(osit)\;
      (object_factory = vtkObjectFactory::GetRegisteredFactories()->GetNextObjectFactory(osit))\;)
  {
    if (object_factory->HasOverride(\"vtkActor\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkActor\", \"vtkWebGPUActor\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkActor\", \"vtkOpenGLActor\")\;
    }
    if (object_factory->HasOverride(\"vtkCamera\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkCamera\", \"vtkWebGPUCamera\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkCamera\", \"vtkOpenGLCamera\")\;
    }
    if (object_factory->HasOverride(\"vtkHardwareSelector\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkHardwareSelector\", \"vtkWebGPUHardwareSelector\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkHardwareSelector\", \"vtkOpenGLHardwareSelector\")\;
    }
    if (object_factory->HasOverride(\"vtkLight\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkLight\", \"vtkWebGPULight\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkLight\", \"vtkOpenGLLight\")\;
    }
    if (object_factory->HasOverride(\"vtkPolyDataMapper\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkPolyDataMapper\", \"vtkWebGPUPolyDataMapper\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkPolyDataMapper\", \"vtkOpenGLPolyDataMapper\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkPolyDataMapper\", \"vtkOpenGLES30PolyDataMapper\")\;
    }
    if (object_factory->HasOverride(\"vtkProperty\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkProperty\", \"vtkWebGPUProperty\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkProperty\", \"vtkOpenGLProperty\")\;
    }
    if (object_factory->HasOverride(\"vtkRenderer\"))
    {
      object_factory->SetEnableFlag(use_webgpu, \"vtkRenderer\", \"vtkWebGPURenderer\")\;
      object_factory->SetEnableFlag(use_opengl, \"vtkRenderer\", \"vtkOpenGLRenderer\")\;
    }
    if (object_factory->HasOverride(\"vtkRenderWindow\"))
    {
      if (use_webgpu && !use_opengl)
      {
        object_factory->SetEnableFlag(use_webgpu, \"vtkRenderWindow\", \"vtkSDL2WebGPURenderWindow\")\;
        // disable ALL opengl render window overrides.
        object_factory->SetEnableFlag(0, \"vtkRenderWindow\", \"vtkCocoaOpenGLRenderWindow\")\;
        object_factory->SetEnableFlag(0, \"vtkRenderWindow\", \"vtkWin32OpenGLRenderWindow\")\;
        object_factory->SetEnableFlag(0, \"vtkRenderWindow\", \"vtkXOpenGLRenderWindow\")\;
        object_factory->SetEnableFlag(0, \"vtkRenderWindow\", \"vtkSDL2OpenGLRenderWindow\")\;
      }
    }
  }"
)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGWEBGPU_EXPORT"
  EXTRA_INCLUDES "<vtkCollection.h>" "<vtkObjectFactoryCollection.h>" "<vtkLogger.h>" "<cstdlib>"
  INITIAL_CODE ${intial_cpp_code})

list (APPEND vtk_wgpu_public_headers
  vtk_wgpu.h
)
list (APPEND vtk_wgpu_public_headers
  vtkWGPUContext.h
)
list (APPEND vtk_wgpu_priv_sources
  vtkWGPUContext.cxx
)

vtk_module_add_module(VTK::RenderingWebGPU
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  SOURCES ${vtk_object_factory_source} ${vtk_wgpu_priv_sources} ${wgsl_shader_sources}
  HEADERS ${vtk_wgpu_public_headers}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${vtk_wgpu_private_headers} ${wgsl_shader_headers}
)

vtk_module_definitions(VTK::RenderingWebGPU
  PUBLIC
    VTK_ENABLE_WEBGPU
)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_compile_features(VTK::RenderingWebGPU
    PRIVATE
      cxx_std_14
  )
  vtk_module_include(VTK::RenderingWebGPU
    INTERFACE
      "${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include/webgpu"
  )
  vtk_module_link_options(VTK::RenderingWebGPU
    INTERFACE
      "SHELL:-s USE_WEBGPU=1"
  )
  vtk_module_find_package(PACKAGE SDL2)
  vtk_module_link(VTK::RenderingWebGPU
    PUBLIC
      SDL2::SDL2)
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
      "utils_metal.mm"
    )
  endif()
  vtk_module_find_package(PACKAGE SDL2)
  vtk_module_find_package(PACKAGE Dawn)
  vtk_module_compile_features(VTK::RenderingWebGPU
    PUBLIC
      cxx_std_14
  )
  # Must include these source files here.
  vtk_module_sources(VTK::RenderingWebGPU PRIVATE
    "${DAWN_GEN_SRC_DIR}/dawn/webgpu_cpp.cpp"
    "${DAWN_GEN_SRC_DIR}/dawn/dawn_proc.c"
  )
  vtk_module_link(VTK::RenderingWebGPU
    PRIVATE
      SDL2::SDL2
      ${DAWN_LIBRARIES}
  )
  vtk_module_include(VTK::RenderingWebGPU
    PUBLIC
      ${DAWN_INCLUDE_DIRS}
  )
  vtk_module_definitions(VTK::RenderingWebGPU
    PUBLIC
      "VTK_USE_DAWN_NATIVE"
  )
  # TODO: dawn webgpu
endif ()
