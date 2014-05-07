find_package(GLEW REQUIRED)

set(Module_SRCS vtkSinglePassVolumeMapper.cxx GLSLShader.cxx)
set_source_files_properties(
  vtkSinglePassVolumeMapper
  GLSLShader
  WRAP_EXCLUDE
)



vtk_module_library(vtkVolume ${Module_SRCS})
target_link_libraries(vtkVolume LINK_PRIVATE ${GLEW_LIBRARY})

add_custom_command(OUTPUT "${vtkVolume_BINARY_DIR}/shaders"
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${vtkVolume_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders")
add_custom_target(copy_shaders ALL DEPENDS "${vtkVolume_BINARY_DIR}/shaders")
add_dependencies(vtkVolume copy_shaders)
