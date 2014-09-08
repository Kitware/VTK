if(TARGET vtkRenderingOpenGL)
  message(FATAL_ERROR "vtkRenderingOpenGL2 cannot be built with vtkRenderingOpenGL, please disable one of them.")
endif()

find_package(OpenGL REQUIRED)
include_directories(SYSTEM
  ${OPENGL_INCLUDE_DIR})

set(${vtk-module}_SYSTEM_INCLUDE_DIRS
  ${OPENGL_INCLUDE_DIR})

include(vtkObjectFactory)

# Copy of what is in vtkRenderingOpenGL for logic on X, Carbon, Cocoa, etc.
set(VTK_USE_X_DEFAULT OFF)

if(APPLE AND NOT APPLE_IOS)
  option(VTK_USE_COCOA "Use Cocoa for VTK render windows" ON)
  option(VTK_USE_CARBON "Use Carbon for VTK render windows (deprecated)" OFF)
  mark_as_advanced(VTK_USE_COCOA VTK_USE_CARBON)
  if(VTK_USE_COCOA AND VTK_USE_CARBON)
    message(FATAL_ERROR "You can't enable Carbon and Cocoa - you must choose one.")
  endif()
elseif(UNIX AND NOT ANDROID AND NOT APPLE_IOS)
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
if(VTK_USE_X OR VTK_USE_CARBON OR VTK_USE_COCOA OR WIN32 OR ANDROID OR APPLE_IOS)
  set(VTK_USE_OSMESA FALSE)
else()
  set(VTK_USE_OSMESA TRUE)
endif()

# The TDx support has not been moved, this header is here to define nothing,
# and so allow classes in other modules to safely include it and keep going.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTDxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h)

set(Module_SRCS
  vtkglBufferObject.cxx
  vtkglVBOHelper.cxx
  vtkShader.cxx
  vtkShaderProgram.cxx
  vtkglVertexArrayObject.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLGlyph3DHelper.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSelector.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLShaderCache.cxx
  vtkOpenGLTexture.cxx
  vtkOpenGLTextureUnitManager.cxx
  vtkRenderPass.cxx
  vtkRenderState.cxx
  vtkTextureObject.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )

if (NOT ANDROID AND NOT APPLE_IOS)
  set(Module_SRCS ${Module_SRCS}
  vtkPixelBufferObject.cxx
  vtkFrameBufferObject.cxx
  )
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
  vtkShader
  vtkShaderProgram
  vtkglVertexArrayObject
  vtkFrameBufferObject
  vtkOpenGLActor
  vtkOpenGLCamera
  vtkOpenGLImageMapper
  vtkOpenGLImageSliceMapper
  vtkOpenGLGlyph3DHelper
  vtkOpenGLGlyph3DMapper
  vtkOpenGLHardwareSelector
  vtkOpenGLLight
  vtkOpenGLPolyDataMapper
  vtkOpenGLPolyDataMapper2D
  vtkOpenGLProperty
  vtkOpenGLRenderer
  vtkOpenGLShaderCache
  vtkOpenGLTexture
  vtkOpenGLTextureUnitManager
  vtkPixelBufferObject
  vtkRenderbuffer
  vtkRenderPass
  vtkRenderState
  vtkTextureObject
  ${vtk-module}ObjectFactory
  WRAP_EXCLUDE)

set_source_files_properties(
  vtkOpenGLRenderWindow
  vtkRenderPass
  vtkRenderState
  ABSTRACT
  )

set(shader_files
  glsl/vtkglPolyData2DVS.glsl
  glsl/vtkglPolyData2DFS.glsl

  glsl/vtkglPolyDataVSNoLighting.glsl
  glsl/vtkglPolyDataFSNoLighting.glsl

  glsl/vtkglPolyDataVSFragmentLit.glsl
  glsl/vtkglPolyDataFSHeadlight.glsl
  glsl/vtkglPolyDataFSLightKit.glsl
  glsl/vtkglPolyDataFSPositionalLights.glsl
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

set_source_files_properties(
  vtkRenderingOpenGL2ObjectFactory
  WRAP_EXCLUDE
  )

# setup factory overrides
set(opengl_overrides
  Actor
  Camera
  HardwareSelector
  ImageMapper
  ImageSliceMapper
  Glyph3DMapper
  Light
  PolyDataMapper
  PolyDataMapper2D
  Property
  Renderer
  Texture
  )
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
  set(vtk_module_vtkRenderWindow_override "vtkOSOpenGL2RenderWindow")
  list(APPEND Module_SRCS vtkOSOpenGL2RenderWindow.cxx)
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
#  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
#  set(vtk_module_vtkRenderWindowInteractor_override "vtkAndroidRenderWindowInteractor")
#  list(APPEND vtk_module_overrides "vtkRenderWindow")
#  set(vtk_module_vtkRenderWindow_override "vtkEGLRenderWindow")
#  list(APPEND Module_SRCS
#    vtkAndroidRenderWindowInteractor.cxx
#    vtkEGLRenderWindow.cxx
#    )
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
elseif(VTK_USE_COCOA)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework Cocoa")
elseif(VTK_USE_CARBON)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework Carbon")
endif()

if(ANDROID)
  target_link_libraries(${vtk-module}
    LINK_PUBLIC ${OPENGL_egl_LIBRARY})
endif()
