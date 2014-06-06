find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Eigen3 REQUIRED)
include_directories(SYSTEM
  ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR})
if(WIN32 AND NOT BUILD_SHARED_LIBS)
  add_definitions(-DGLEW_STATIC)
endif()

set(${vtk-module}_SYSTEM_INCLUDE_DIRS
  ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS})

set(Module_SRCS
  vtkglBufferObject.cxx
  vtkglVBOHelper.cxx
  vtkglShader.cxx
  vtkglShaderProgram.cxx
  vtkglTexture2D.cxx
  vtkglVertexArrayObject.cxx
  vtkOpenGL2Actor.cxx
  vtkOpenGL2Camera.cxx
  vtkOpenGL2ImageSliceMapper.cxx
  vtkOpenGL2Glyph3DMapper.cxx
  vtkOpenGL2HardwareSelector.cxx
  vtkOpenGL2Light.cxx
  vtkOpenGL2PolyDataMapper2D.cxx
  vtkOpenGL2Property.cxx
  vtkVBOPolyDataMapper.cxx
  vtkOpenGL2RenderWindow.cxx
  vtkOpenGL2Renderer.cxx
  vtkOpenGL2ShaderCache.cxx
  vtkOpenGL2Texture.cxx
  vtkOpenGL2TextureUnitManager.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
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
  vtkOpenGL2RenderWindow
  ABSTRACT
  )


set_source_files_properties(
  vtkRenderingOpenGL2ObjectFactory
  WRAP_EXCLUDE
  )

# setup factory overrides
if(VTK_REPLACE_OPENGL_OVERRIDES)
  set(opengl_overrides
    Actor
    Camera
    HardwareSelector
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
    vtk_add_override(vtk${_override} vtkOpenGL2${_override})
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
  set(vtk_module_vtkRenderWindowInteractor_override "vtkXOpenGL2RenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkXOpenGL2RenderWindow")
  list(APPEND Module_SRCS
    vtkXOpenGL2RenderWindowInteractor.cxx
    vtkXOpenGL2RenderWindow.cxx
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
  set(vtk_module_vtkRenderWindowInteractor_override "vtkWin32OpenGL2RenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkWin32OpenGL2RenderWindow")
  list(APPEND Module_SRCS
    vtkWin32OpenGL2RenderWindowInteractor.cxx
    vtkWin32OpenGL2RenderWindow.cxx
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
endif() # VTK_REPLACE_OPENGL_OVERRIDES

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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGL2ObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)


vtk_module_library(${vtk-module} ${Module_SRCS})
target_link_libraries(${vtk-module}
  LINK_PRIVATE ${GLEW_LIBRARY}
  LINK_PUBLIC  ${OPENGL_LIBRARIES})
if(VTK_USE_X)
  target_link_libraries(${vtk-module}
    LINK_PUBLIC ${X11_LIBRARIES} ${X11_Xt_LIB})
endif()
