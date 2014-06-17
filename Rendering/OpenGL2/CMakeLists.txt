if(TARGET vtkRenderingOpenGL)
  message(FATAL_ERROR "vtkRenderingOpenGL2 cannot be built with vtkRenderingOpenGL, please disable one of them.")
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
include_directories(SYSTEM
  ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS})
if(WIN32 AND NOT BUILD_SHARED_LIBS)
  add_definitions(-DGLEW_STATIC)
endif()

# Now we need to generate the object factory for this module. Create a list of
# overrides, and then generate the class that overrides those classes.
macro(vtk_add_override base override)
  list(APPEND vtk_module_overrides ${base})
  set(vtk_module_${base}_override ${override})
endmacro()

set(${vtk-module}_SYSTEM_INCLUDE_DIRS
  ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS})

# Copy of what is in vtkRenderingOpenGL for logic on X, Carbon, Cocoa, etc.
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
  vtkglBufferObject.cxx
  vtkglVBOHelper.cxx
  vtkglShader.cxx
  vtkglShaderProgram.cxx
  vtkglVertexArrayObject.cxx
  vtkFrameBufferObject.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSelector.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkVBOPolyDataMapper.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLShaderCache.cxx
  vtkOpenGLTexture.cxx
  vtkOpenGLTextureUnitManager.cxx
  vtkPixelBufferObject.cxx
  vtkRenderbuffer.cxx
  vtkRenderPass.cxx
  vtkRenderState.cxx
  vtkTextureObject.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )

set(${vtk-module}_HDRS
  vtkOpenGL.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h
  )

set(shader_files
  # lighting in the VS
  glsl/vtkglPolyData2DVS.glsl
  glsl/vtkglPolyData2DFS.glsl
  glsl/vtkglPolyDataVSLightKit.glsl
  glsl/vtkglPolyDataVSHeadlight.glsl
  glsl/vtkglPolyDataVSPositionalLights.glsl
  glsl/vtkglPolyDataVSNoLighting.glsl
  glsl/vtkglPolyDataFS.glsl

  # lighting in the FS
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
endif()

set_source_files_properties(
  vtkOpenGLRenderWindow
  ABSTRACT
  )

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
  PolyDataMapper2D
  Property
  Renderer
  Texture
  )
vtk_add_override(vtkPolyDataMapper vtkVBOPolyDataMapper)
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
  message("VTK will be built with OSMesa support!")
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
endif()

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

# Detect and clear OpenGL errors. If not then other classes/user code
# may detect VTK's OpenGL errors and vice-versa.
set(VTK_REPORT_OPENGL_ERRORS ON CACHE BOOL "Enable OpenGL error check and report")
mark_as_advanced(VTK_REPORT_OPENGL_ERRORS)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGLError.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h)

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGL2ObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h)
# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkRenderingOpenGLConfigure.h\"")

vtk_module_library(${vtk-module} ${Module_SRCS})
target_link_libraries(${vtk-module}
  LINK_PRIVATE ${GLEW_LIBRARY}
  LINK_PUBLIC  ${OPENGL_LIBRARIES})
if(VTK_USE_X)
  target_link_libraries(${vtk-module}
    LINK_PUBLIC ${X11_LIBRARIES} ${X11_Xt_LIB})
endif()
