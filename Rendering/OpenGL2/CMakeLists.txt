if(TARGET vtkRenderingOpenGL)
  message(FATAL_ERROR "vtkRenderingOpenGL2 cannot be built with vtkRenderingOpenGL, please disable one of them.")
endif()

find_package(OpenGL REQUIRED)
include_directories(SYSTEM
  ${OPENGL_INCLUDE_DIR})

set(${vtk-module}_SYSTEM_INCLUDE_DIRS
  ${OPENGL_INCLUDE_DIR})

include(vtkObjectFactory)
include(vtkOpenGLMesaLogic)

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
  vtkDepthPeelingPass.cxx
  vtkGaussianBlurPass.cxx
  vtkGenericCompositePolyDataMapper2.cxx
  vtkImageProcessingPass.cxx
  vtkLightsPass.cxx
  vtkOpaquePass.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLGlyph3DHelper.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSelector.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPointGaussianMapper.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLShaderCache.cxx
  vtkOpenGLTexture.cxx
  vtkOverlayPass.cxx
  vtkRenderPass.cxx
  vtkRenderStepsPass.cxx
  vtkRenderPassCollection.cxx
  vtkRenderState.cxx
  vtkSequencePass.cxx
  vtkShader.cxx
  vtkShaderProgram.cxx
  vtkSobelGradientMagnitudePass.cxx
  vtkTextureObject.cxx
  vtkTextureUnitManager.cxx
  vtkTranslucentPass.cxx
  vtkVolumetricPass.cxx
  vtkglBufferObject.cxx
  vtkglVBOHelper.cxx
  vtkglVertexArrayObject.cxx
  )

if (NOT DEFINED OPENGL_ES_VERSION)
  set(Module_SRCS ${Module_SRCS}
    vtkFrameBufferObject.cxx
    vtkPixelBufferObject.cxx
    )
else()
  if (${OPENGL_ES_VERSION} MATCHES 3.0)
    set(Module_SRCS ${Module_SRCS}
      vtkFrameBufferObject.cxx
      )
  endif()
endif()

set(${vtk-module}_HDRS
  vtkOpenGL.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h
  )

set_source_files_properties(
  vtkglBufferObject
  vtkglVBOHelper
  vtkglVertexArrayObject
  vtkFrameBufferObject
  vtkOpenGLGlyph3DHelper
  vtkOpenGLGlyph3DMapper
  vtkOpenGLShaderCache
  vtkPixelBufferObject
  vtkRenderbuffer
  vtkRenderState
  vtkTextureObject
  vtkTextureUnitManager
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  WRAP_EXCLUDE)

set_source_files_properties(
  vtkOpenGLRenderWindow
  vtkRenderPass
  vtkRenderState
  ABSTRACT
  )

set(shader_files
  glsl/vtkDepthPeelingPassFinalFS.glsl
  glsl/vtkDepthPeelingPassIntermediateFS.glsl
  glsl/vtkGaussianBlurPassFS.glsl
  glsl/vtkGaussianBlurPassVS.glsl
  glsl/vtkSobelGradientMagnitudePass1FS.glsl
  glsl/vtkSobelGradientMagnitudePass2FS.glsl
  glsl/vtkPointGaussianVS.glsl
  glsl/vtkTextureObjectFS.glsl
  glsl/vtkTextureObjectVS.glsl
  glsl/vtkglGlyph3DVSFragmentLit.glsl
  glsl/vtkglPolyData2DFS.glsl
  glsl/vtkglPolyData2DVS.glsl
  glsl/vtkglPolyDataFSHeadlight.glsl
  glsl/vtkglPolyDataFSLightKit.glsl
  glsl/vtkglPolyDataFSNoLighting.glsl
  glsl/vtkglPolyDataFSPositionalLights.glsl
  glsl/vtkglPolyDataVSFragmentLit.glsl
  glsl/vtkglPolyDataVSNoLighting.glsl
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
    ARGS ${res} ${src} ${file_we}
      --build-header VTKRENDERINGOPENGL2_EXPORT vtkRenderingOpenGL2Module.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

if(VTK_USE_X)
  find_package(X11 REQUIRED)
  if(NOT X11_Xt_FOUND)
    message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
  endif()
  include_directories(${X11_INCLUDE_DIR} ${X11_Xt_INCLUDE_PATH})
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkAbstractInteractionDevice")
  set(vtk_module_vtkAbstractInteractionDevice_override "vtkXInteractionDevice")
  list(APPEND vtk_module_overrides "vtkAbstractRenderDevice")
  set(vtk_module_vtkAbstractRenderDevice_override "vtkXOpenGLRenderDevice")
  list(APPEND Module_SRCS
    vtkXInteractionDevice.cxx
    vtkXOpenGLRenderDevice.cxx
    )
  set_source_files_properties(
    vtkXInteractionDevice
    vtkXOpenGLRenderDevice
    WRAP_EXCLUDE)
endif()

# setup factory overrides
set(opengl_overrides
  Actor
  Camera
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
  Texture
  )

# Now to see about the GPU info list.
list(APPEND vtk_module_overrides "vtkGPUInfoList")
list(APPEND Module_SRCS "vtkDummyGPUInfoList.cxx")
set(vtk_module_vtkGPUInfoList_override "vtkDummyGPUInfoList")

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()

# FIXME: Not sure that this is the best way to handle X...
if(VTK_USE_X)
  find_package(X11 REQUIRED)
  if(NOT X11_Xt_FOUND)
    message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
  endif()
  include_directories(${X11_INCLUDE_DIR} ${X11_Xt_INCLUDE_PATH})
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkXOpenGLRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkXOpenGLRenderWindow")
  list(APPEND Module_SRCS
    vtkXOpenGLRenderWindowInteractor.cxx
    vtkXOpenGLRenderWindow.cxx
    )
elseif(VTK_USE_OSMESA)
  message(STATUS "VTK will be built with OSMesa support!")
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
  set(vtk_module_vtkRenderWindowInteractor_override "vtkWin32OpenGLRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkWin32OpenGLRenderWindow")
  list(APPEND Module_SRCS
    vtkWin32OpenGLRenderWindowInteractor.cxx
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
elseif(ANDROID)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkAndroidRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkEGLRenderWindow")
  list(APPEND Module_SRCS
    vtkAndroidRenderWindowInteractor.cxx
    vtkEGLRenderWindow.cxx
    )
  include_directories(
    "${ANDROID_NDK}/platforms/android-${ANDROID_NATIVE_API_LEVEL}/arch-${ANDROID_ARCH_NAME}/usr/include"
    "${ANDROID_NDK}/sources/android/native_app_glue"
    )
elseif(APPLE_IOS)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkIOSRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkIOSRenderWindow")
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

vtk_object_factory_configure("${vtk_module_overrides}")

# Detect and clear OpenGL errors. If not then other classes/user code
# may detect VTK's OpenGL errors and vice-versa.
set(VTK_REPORT_OPENGL_ERRORS ON CACHE BOOL "Enable OpenGL error check and report")
mark_as_advanced(VTK_REPORT_OPENGL_ERRORS)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGLError.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h)

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h)
# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkRenderingOpenGLConfigure.h\"")

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_module_link_libraries(${vtk-module}
  LINK_PUBLIC  ${OPENGL_LIBRARIES})
if(VTK_USE_X)
  vtk_module_link_libraries(${vtk-module}
    LINK_PUBLIC ${X11_LIBRARIES} ${X11_Xt_LIB})
elseif(VTK_USE_OSMESA)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC ${OSMESA_LIBRARY})
elseif(VTK_USE_COCOA)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework Cocoa")
elseif(VTK_USE_CARBON)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework Carbon")
endif()

if(ANDROID)
  target_link_libraries(${vtk-module}
    LINK_PUBLIC ${OPENGL_egl_LIBRARY})
endif()

if (APPLE_IOS)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework UIKit")
endif()
