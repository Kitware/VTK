find_package(GLEW REQUIRED)

 # Source files
set(Module_SRCS
  vtkGPUVolumeRayCastMapper.cxx
  vtkOpenGLGPUVolumeRayCastMapper
  vtkGLSLShader.cxx)

set_source_files_properties(
  vtkGPUVolumeRayCastMapper
  vtkGLSLShader
  WRAP_EXCLUDE)

set(opengl_overrides
  GPUVolumeRayCastMapper
  )
foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()

set(shader_files
    shaders/raycasterfs.glsl
    shaders/raycastervs.glsl)

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
      --build-header VTKVOLUME_EXPORT vtkVolumeModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkVolume ${Module_SRCS})
target_link_libraries(vtkVolume LINK_PRIVATE ${GLEW_LIBRARY})

add_custom_command(OUTPUT "${vtkVolume_BINARY_DIR}/shaders"
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${vtkVolume_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders")
add_custom_target(copy_shaders ALL DEPENDS "${vtkVolume_BINARY_DIR}/shaders")
add_dependencies(vtkVolume copy_shaders)
