# This module cannot be built at the same time as vtkRenderingOpenGL2
if(TARGET vtkRenderingOpenGL2)
  message(FATAL_ERROR "vtkRenderingOpenGL cannot be built with vtkRenderingOpenGL2, please disable one of them.")
endif()

# FIXME: The TDx support needs some refactoring before we can build it in a
# modular fashion, for now it must remain an option on vtkRenderingOpenGL.
option(VTK_USE_TDX "Use TDx interaction devices" OFF)
mark_as_advanced(VTK_USE_TDX)
if (TDX_INCLUDE_DIR)
  include_directories(${TDX_INCLUDE_DIR})
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTDxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h)

include(vtkObjectFactory)

set(Module_SRCS
  vtkCameraPass.cxx
  vtkChooserPainter.cxx
  vtkClearRGBPass.cxx
  vtkClearZPass.cxx
  vtkClipPlanesPainter.cxx
  vtkCoincidentTopologyResolutionPainter.cxx
  vtkColorMaterialHelper.cxx
  vtkCompositePainter.cxx
  vtkCompositePolyDataMapper2.cxx
  vtkDataTransferHelper.cxx
  vtkDefaultPainter.cxx
  vtkDefaultPass.cxx
  vtkDepthPeelingPass.cxx
  vtkDisplayListPainter.cxx
  vtkFrameBufferObject.cxx
  vtkFrameBufferObject2.cxx
  vtkGLSLShaderDeviceAdapter2.cxx
  vtkGaussianBlurPass.cxx
  vtkGenericOpenGLRenderWindow.cxx
  vtkHiddenLineRemovalPass.cxx
  vtkHardwareSelectionPolyDataPainter.cxx # Needed by vtkPainterPolyDataMapper
  vtkImageProcessingPass.cxx
  vtkLightingHelper.cxx
  vtkLightingPainter.cxx
  vtkLightsPass.cxx
  vtkLinesPainter.cxx
  vtkOpaquePass.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLClipPlanesPainter.cxx
  vtkOpenGLCoincidentTopologyResolutionPainter.cxx
  vtkOpenGLCompositePainter.cxx
  vtkOpenGLDisplayListPainter.cxx
  vtkOpenGLExtensionManager.cxx
  vtkOpenGLGL2PSHelper.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSelector.cxx
  vtkOpenGLHardwareSupport.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLLabeledContourMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLLightMonitor.cxx
  vtkOpenGLLightingPainter.cxx
  vtkOpenGLModelViewProjectionMonitor.cxx
  vtkOpenGLPainterDeviceAdapter.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLRepresentationPainter.cxx
  vtkOpenGLScalarsToColorsPainter.cxx
  vtkOpenGLState.cxx
  vtkOpenGLTexture.cxx
  vtkOverlayPass.cxx
  vtkPainter.cxx
  vtkPainterPolyDataMapper.cxx
  vtkPixelBufferObject.cxx
  vtkPointsPainter.cxx
  vtkPolyDataPainter.cxx
  vtkPolygonsPainter.cxx
  vtkPrimitivePainter.cxx
  vtkRenderPassCollection.cxx
  vtkRenderStepsPass.cxx
  vtkRenderbuffer.cxx
  vtkRepresentationPainter.cxx
  vtkScalarsToColorsPainter.cxx
  vtkSequencePass.cxx
  vtkShader2.cxx
  vtkShader2Collection.cxx
  vtkShaderProgram2.cxx
  vtkShadowMapBakerPass.cxx
  vtkShadowMapPass.cxx
  vtkSobelGradientMagnitudePass.cxx
  vtkStandardPolyDataPainter.cxx
  vtkTStripsPainter.cxx
  vtkTextureObject.cxx
  vtkTextureUnitManager.cxx
  vtkTranslucentPass.cxx
  vtkUniformVariables.cxx
  vtkValuePainter.cxx
  vtkValuePass.cxx
  vtkValuePasses.cxx
  vtkVolumetricPass.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLObjectFactory.cxx
  )
if (NOT VTK_LEGACY_REMOVE)
  # vtkOpenGLPolyDataMapper is only added if not removing legacy code.
  list(APPEND Module_SRCS
    vtkOpenGLPolyDataMapper.cxx)
endif()

if(VTK_USE_TDX)
  list(APPEND Module_SRCS
    vtkTDxDevice.cxx
    vtkTDxInteractorStyleGeo.cxx)

  if(WIN32)
   list(APPEND Module_SRCS vtkTDxWinDevice.cxx)
  endif()
  if(UNIX)
    list(APPEND Module_SRCS vtkTDxUnixDevice.cxx)
  endif()
  if(APPLE)
    list(APPEND Module_SRCS vtkTDxMacDevice.cxx)
  endif()
endif()


set(${vtk-module}_HDRS
  vtkgluPickMatrix.h
  vtkOpenGL.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkgl.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h
  )

set_source_files_properties(
  vtkImageProcessingPass
  vtkOpenGLRenderWindow
  vtkPainter
  vtkPolyDataPainter
  vtkPrimitivePainter
  ABSTRACT
  )

set_source_files_properties(
  vtkLightingHelper
  vtkOpenGLState
  vtkOpenGLGL2PSHelper
  vtkShader2
  vtkShader2Collection
  vtkShaderProgram2
  vtkTextureUnitManager
  vtkgl
  vtkRenderingOpenGLObjectFactory
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkgl
  vtkLightingHelper
  vtkOpenGLState
  vtkOpenGLGL2PSHelper
  vtkRenderingOpenGLObjectFactory
  PROPERTIES WRAP_EXCLUDE_PYTHON 1
  )

include(vtkOpenGL)

# Configuration for vtkOpenGLExtensionManager
if(WIN32)
  set(VTK_USE_WGL_GET_PROC_ADDRESS 1)
  find_package(DirectX QUIET)
  if(DirectX_FOUND)
    set(VTK_USE_DIRECTX 1)
  endif()
elseif(APPLE AND NOT VTK_USE_OSMESA)
  set(VTK_USE_APPLE_LOADER 1)
  set(VTK_USE_CORE_GRAPHICS 1)
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
  elseif (VTK_USE_OSMESA)
    # we'll use OSMesa to load extensions.
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

set(shader_files
  vtkColorMaterialHelper_vs.glsl
  vtkDepthPeeling_fs.glsl
  vtkGaussianBlurPassShader_fs.glsl
  vtkLighting_s.glsl # s not vs or fs because it can be used in both.
  vtkLightingHelper_s.glsl # s not vs or fs because it can be used in both.
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
  set_source_files_properties(${file_we}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()

set(opengl_overrides
  Actor
  Camera
  ClipPlanesPainter
  CoincidentTopologyResolutionPainter
  CompositePainter
  DisplayListPainter
  Glyph3DMapper
  HardwareSelector
  ImageSliceMapper
  LabeledContourMapper
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
  endif()
  set_source_files_properties(
    vtkCocoaGLView
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
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
  set(extra_libs)
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

# Now lets create the object factory classes
vtk_object_factory_configure("${vtk_module_overrides}")

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h)

# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkRenderingOpenGLConfigure.h\"")

# Since several headers in this module include vtkOpenGL.h which needed GL.h,
# need to ensure that any dependent modules inherit the include directory
# automatically.
if(VTK_USE_OSMESA)
  set(${vtk-module}_SYSTEM_INCLUDE_DIRS
    ${OSMESA_INCLUDE_DIR})
else()
  set(${vtk-module}_SYSTEM_INCLUDE_DIRS
    ${OPENGL_INCLUDE_DIR})
endif()

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

# For testing new driver releases
set(VTK_IGNORE_GLDRIVER_BUGS
    OFF CACHE BOOL
   "Enable buggy OpenGL drivers for testing.")
mark_as_advanced(VTK_IGNORE_GLDRIVER_BUGS)
if (VTK_IGNORE_GLDRIVER_BUGS)
  set_property(SOURCE
     vtkOpenGLExtensionManager.cxx
     APPEND PROPERTY
     COMPILE_DEFINITIONS
     VTK_IGNORE_GLDRIVER_BUGS)
endif()

vtk_module_library(vtkRenderingOpenGL ${Module_SRCS})
vtk_opengl_link(vtkRenderingOpenGL)
vtk_module_link_libraries(vtkRenderingOpenGL
  LINK_PUBLIC ${extra_libs})
if(VTK_USE_X)
  vtk_module_link_libraries(vtkRenderingOpenGL
    LINK_PUBLIC ${X11_LIBRARIES} ${X11_Xt_LIB})
elseif(VTK_USE_COCOA)
  vtk_module_link_libraries(vtkRenderingOpenGL LINK_PUBLIC "-framework Cocoa")
endif()
