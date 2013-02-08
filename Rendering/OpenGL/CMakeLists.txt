# FIXME: This should probably be fixed, vtkgl generation needs some refinement
# to allow for things to split up properly.

set(VTK_USE_X_DEFAULT OFF)

if(APPLE)
  option(VTK_USE_COCOA "Use Cocoa for VTK render windows" ON)
  option(VTK_USE_CARBON "Use Carbon for VTK render windows (deprecated)" OFF)
  mark_as_advanced(VTK_USE_COCOA VTK_USE_CARBON)
  if(VTK_USE_COCOA AND VTK_USE_CARBON)
    message(FATAL_ERROR "You can't enable Carbon and Cocoa - you must choose one.")
  endif()
elseif(UNIX)
  set(VTK_USE_X_DEFAULT ON)
endif()

option(VTK_USE_X "Use X for VTK render windows" ${VTK_USE_X_DEFAULT})
mark_as_advanced(VTK_USE_X)

# OSMesa logic for offscreen mesa rendering.
option(VTK_OPENGL_HAS_OSMESA
  "The OpenGL library being used supports off screen Mesa calls" OFF)
option(VTK_USE_OFFSCREEN "Use off screen calls by default" OFF)
unset(VTK_CAN_DO_OFFSCREEN)
if(VTK_OPENGL_HAS_OSMESA OR WIN32)
  set(VTK_CAN_DO_OFFSCREEN 1)
endif()
mark_as_advanced(VTK_OPENGL_HAS_OSMESA VTK_USE_OFFSCREEN)
if(VTK_USE_X OR VTK_USE_CARBON OR VTK_USE_COCOA OR WIN32)
  set(VTK_USE_OSMESA FALSE)
else()
  set(VTK_USE_OSMESA TRUE)
endif()

# FIXME: The TDx support needs some refactoring before we can build it in a
# modular fashion, for now it must remain an option on vtkRenderingOpenGL.
option(VTK_USE_TDX "Use TDx interaction devices" OFF)
mark_as_advanced(VTK_USE_TDX)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTDxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h)

set(Module_SRCS
  vtkCameraPass.cxx
  vtkClearZPass.cxx
  vtkColorMaterialHelper.cxx
  vtkDataTransferHelper.cxx
  vtkDefaultPass.cxx
  vtkDepthPeelingPass.cxx
  vtkFrameBufferObject.cxx
  vtkGaussianBlurPass.cxx
  vtkGenericOpenGLRenderWindow.cxx
  vtkImageProcessingPass.cxx
  vtkLightingHelper.cxx
  vtkLightsPass.cxx
  vtkLineIntegralConvolution2D.cxx
  vtkOpaquePass.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLClipPlanesPainter.cxx
  vtkOpenGLCoincidentTopologyResolutionPainter.cxx
  vtkOpenGLDisplayListPainter.cxx
  vtkOpenGLExtensionManager.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSupport.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLLightingPainter.cxx
  vtkOpenGLPainterDeviceAdapter.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRepresentationPainter.cxx
  vtkOpenGLScalarsToColorsPainter.cxx
  vtkOpenGLState.cxx
  vtkOpenGLTexture.cxx
  vtkOverlayPass.cxx
  vtkPixelBufferObject.cxx
  vtkRenderPassCollection.cxx
  vtkRenderPass.cxx
  vtkRenderState.cxx
  vtkSequencePass.cxx
  vtkShader2Collection.cxx
  vtkShader2.cxx
  vtkShaderProgram2.cxx
  vtkShadowMapBakerPass.cxx
  vtkShadowMapPass.cxx
  vtkSobelGradientMagnitudePass.cxx
  vtkTextureObject.cxx
  vtkTextureUnitManager.cxx
  vtkTranslucentPass.cxx
  vtkUniformVariables.cxx
  vtkVolumetricPass.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.cxx
  vtkGLSLShaderDeviceAdapter.cxx
  vtkGLSLShaderDeviceAdapter2.cxx
  vtkGLSLShader.cxx
  vtkGLSLShaderProgram.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLObjectFactory.cxx
  )

set(${vtk-module}_HDRS
  vtkgluPickMatrix.h
  vtkOpenGL.h
  vtkOpenGLProjectedPolyDataRayBounder.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.h
  )

set_source_files_properties(
  vtkImageProcessingPass
  vtkOpenGLRenderWindow
  vtkRenderPass
  vtkRenderState
  ABSTRACT
  )

set_source_files_properties(
  vtkLightingHelper
  vtkRenderState
  vtkOpenGLState
  vtkShader2
  vtkShader2Collection
  vtkShaderProgram2
  vtkTextureUnitManager
  vtkgl
  vtkRenderingOpenGLObjectFactory
  WRAP_EXCLUDE
  )

if(VTK_USE_OSMESA)
  find_package(OpenGL)
else()
  find_package(OpenGL REQUIRED)
endif()

# Configuration for vtkOpenGLExtensionManager
if(WIN32)
  set(VTK_USE_WGL_GET_PROC_ADDRESS 1)
  find_package(DirectX QUIET)
  if(DirectX_FOUND)
    set(VTK_USE_DIRECTX 1)
  endif()
elseif(APPLE)
  set(VTK_USE_APPLE_LOADER 1)
  find_package(ApplicationServices)
  find_package(IOKit)
  mark_as_advanced(ApplicationServices IOKit)
  if(ApplicationServices_FOUND AND IOKit_FOUND)
    set(VTK_USE_CORE_GRAPHICS 1)
  endif()
else()
  include(CheckFunctionExists)
  set(VTK_SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
  set(CMAKE_REQUIRED_LIBRARIES
    ${OPENGL_LIBRARIES}
    ${CMAKE_THREAD_LIBS}
    ${CMAKE_REQUIRED_LIBRARIES}
    -lm
    )
  if(VTK_USE_X)
    include(vtkTestGLX)
    check_function_exists(glXGetProcAddressARB VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
    if(VTK_USE_GLX_GET_PROC_ADDRESS_ARB)
      if(NOT "${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES}" STREQUAL "${OPENGL_INCLUDE_DIR}")
        # The OpenGL includes changed.  Retest glXGetProcAddressARB prototype.
        set(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED 0)
      endif()
      if(NOT VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED)
        message(STATUS "Looking for glXGetProcAddressARB in GL/glx.h")
        try_compile(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS
          ${CMAKE_BINARY_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/CheckglXGetProcAddressARB.cxx
          CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}"
          "-DINCLUDE_DIRECTORIES:STRING=${OPENGL_INCLUDE_DIR}"
          OUTPUT_VARIABLE OUTPUT)
        message(STATUS "Looking for glXGetProcAddressARB in GL/glx.h - ${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS}")
        file(APPEND ${CMAKE_BINARY_DIR}/CMakeOutput.log
          "Determining if glXGetProcAddressARB exists in GL/glx.h ${VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS} with the following output:\n"
          "${OUTPUT}\n\n")
        set(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_TESTED 1
          CACHE INTERNAL "Already set VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS")
        set(VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS_INCLUDES
          "${OPENGL_INCLUDE_DIR}"
          CACHE INTERNAL "OpenGL includes used to test glXGetProcAddressARB prototype.")
      endif()
      if(NOT VTK_GLX_GET_PROC_ADDRESS_ARB_PROTOTYPE_EXISTS)
        set(VTK_DEFINE_GLX_GET_PROC_ADDRESS_PROTOTYPE 1)
      endif()
    else()
      check_function_exists(glXGetProcAddress VTK_USE_GLX_GET_PROC_ADDRESS)
      if(NOT VTK_USE_GLX_GET_PROC_ADDRESS)
        message(STATUS "Could not find extension loader.  Extensions disabled.")
        set(VTK_NO_EXTENSION_LOADING 1)
      endif()
    endif()
    find_package(NVCtrlLib QUIET)
    mark_as_advanced(NVCtrlLib_INCLUDE_DIR NVCtrlLib_LIBRARY)
    if(NVCtrlLib_FOUND)
      set(VTK_USE_NVCONTROL 1)
    endif()
  else()
    set(VTK_NO_EXTENSION_LOADING 1)
  endif()
  set(CMAKE_REQUIRED_LIBRARIES "${VTK_SAVE_CMAKE_REQUIRED_LIBRARIES}")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGLExtensionManagerConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLExtensionManagerConfigure.h)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.cxx ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.h
  COMMAND vtkParseOGLExt
  ARGS ${CMAKE_CURRENT_BINARY_DIR} ${VTK_GLEXT_FILE} ${VTK_GLXEXT_FILE} ${VTK_WGLEXT_FILE}
  DEPENDS ${VTK_GLEXT_FILE} ${VTK_GLXEXT_FILE} ${VTK_WGLEXT_FILE}
  )

include_directories(${OPENGL_INCLUDE_DIR})

set(shader_files
  vtkColorMaterialHelper_vs.glsl
  vtkDepthPeeling_fs.glsl
  vtkGaussianBlurPassShader_fs.glsl
  vtkLighting_s.glsl # s not vs or fs because it can be used in both.
  vtkLightingHelper_s.glsl # s not vs or fs because it can be used in both.
  vtkLineIntegralConvolution2D_fs.glsl
  vtkLineIntegralConvolution2D_fs1.glsl
  vtkLineIntegralConvolution2D_fs2.glsl
  vtkOpenGLRenderer_PeelingFS.glsl
  vtkOpenGLPropertyDefaultPropFunc_fs.glsl
  vtkOpenGLPropertyDefaultPropFunc_vs.glsl
  vtkOpenGLPropertyDefaultMain_fs.glsl
  vtkOpenGLPropertyDefaultMain_vs.glsl
  vtkSobelGradientMagnitudePassShader1_fs.glsl
  vtkSobelGradientMagnitudePassShader2_fs.glsl
  vtkShadowMapPassShader_fs.glsl
  vtkShadowMapPassShader_vs.glsl
  )

unset(shader_h_files)
foreach(file ${shader_files})
  get_filename_component(file_we ${file} NAME_WE)
  set(src  ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  set(res  ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx)
  set(resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h)
  list(APPEND shader_h_files ${resh})
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file_we} --build-header VTKRENDERINGOPENGL_EXPORT vtkRenderingOpenGLModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

# Now we need to generate the object factory for this module. Create a list of
# overrides, and then generate the class that overrides those classes.
macro(vtk_add_override base override)
  list(APPEND vtk_module_overrides ${base})
  set(vtk_module_${base}_override ${override})
endmacro()

set(opengl_overrides
  Actor
  Camera
  ClipPlanesPainter
  CoincidentTopologyResolutionPainter
  DisplayListPainter
  Glyph3DMapper
  ImageSliceMapper
  Light
  LightingPainter
  PainterDeviceAdapter
  Property
  Renderer
  RepresentationPainter
  ScalarsToColorsPainter
  Texture
  # Some from the vtkImagingFactory class
  ImageMapper
  PolyDataMapper2D
  )

# Some that do not follow the naming scheme...
list(APPEND vtk_module_overrides "vtkPolyDataMapper")
set(vtk_module_vtkPolyDataMapper_override "vtkPainterPolyDataMapper")

# FIXME: Not sure that this is the best way to handle X...
if(VTK_USE_X)
  find_package(X11 REQUIRED)
  if(NOT X11_Xt_FOUND)
    message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
  endif()
  include_directories(${X11_INCLUDE_DIR} ${X11_Xt_INCLUDE_PATH})
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkXRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkXOpenGLRenderWindow")
  list(APPEND Module_SRCS
    vtkXRenderWindowInteractor.cxx
    vtkXOpenGLRenderWindow.cxx
    )
elseif(VTK_USE_OSMESA)
  message("VTK will be built with OSMesa support!")
  find_package(OSMesa REQUIRED)
  if(NOT OSMESA_FOUND)
    message(FATAL_ERROR "The OSMesa library could not be found. REQUIRED for off screen rendering")
  endif()
  include_directories(${OSMESA_INCLUDE_DIR})
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkOSOpenGLRenderWindow")
  list(APPEND Module_SRCS vtkOSOpenGLRenderWindow.cxx)
elseif(WIN32)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkWin32RenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkWin32OpenGLRenderWindow")
  list(APPEND Module_SRCS
    vtkWin32RenderWindowInteractor.cxx
    vtkWin32OpenGLRenderWindow.cxx
    )
elseif(VTK_USE_COCOA)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkCocoaRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkCocoaRenderWindow")
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
  endif(VTK_REQUIRED_OBJCXX_FLAGS)
  set_source_files_properties(vtkCocoaGLView WRAP_EXCLUDE)
elseif(VTK_USE_CARBON)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkCarbonRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkCarbonRenderWindow")
  list(APPEND Module_SRCS
    vtkCarbonRenderWindowInteractor.cxx
    vtkCarbonRenderWindow.cxx
    )
endif()

# Now to see about the GPU info list.
list(APPEND vtk_module_overrides "vtkGPUInfoList")
if(VTK_USE_NVCONTROL)
  list(APPEND Module_SRCS "vtkXGPUInfoList.cxx")
  set(extra_libs ${NVCtrlLib_LIBRARY})
  include_directories(${NVCtrlLib_INCLUDE_DIR})
  set(vtk_module_vtkGPUInfoList_override "vtkXGPUInfoList")
elseif(VTK_USE_CORE_GRAPHICS)
  list(APPEND Module_SRCS "vtkCoreGraphicsGPUInfoList.cxx")
  set(extra_libs ${ApplicationServices_LIBRARY} ${IOKit_LIBRARY})
  include_directories(${ApplicationServices_INCLUDE_DIR} ${IOKit_INCLUDE_DIR})
  set(vtk_module_vtkGPUInfoList_override "vtkCoreGraphicsGPUInfoList")
elseif(VTK_USE_DIRECTX)
  list(APPEND Module_SRCS "vtkDirectXGPUInfoList.cxx")
  set(extra_libs ${DirectX_LIBRARY} wbemuuid)
  include_directories(${DirectX_INCLUDE_DIR})
  set(vtk_module_vtkGPUInfoList_override "vtkDirectXGPUInfoList")
else()
  # Shouldn't this just be the base class, and the others override if needed?
  list(APPEND Module_SRCS "vtkDummyGPUInfoList.cxx")
  set(vtk_module_vtkGPUInfoList_override "vtkDummyGPUInfoList")
endif()

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h)

# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkRenderingOpenGLConfigure.h\"")

vtk_module_library(vtkRenderingOpenGL ${Module_SRCS})
target_link_libraries(vtkRenderingOpenGL ${OPENGL_LIBRARIES} ${extra_libs})
if(VTK_USE_X)
  target_link_libraries(vtkRenderingOpenGL ${X11_LIBRARIES} ${X11_Xt_LIB})
elseif(VTK_USE_COCOA)
  target_link_libraries(vtkRenderingOpenGL "-framework Cocoa")
elseif(VTK_USE_CARBON)
  target_link_libraries(vtkRenderingOpenGL "-framework Carbon")
endif()

if(VTK_OPENGL_HAS_OSMESA)
  target_link_libraries(vtkRenderingOpenGL ${OSMESA_LIBRARY})
endif()
