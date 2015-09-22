include(vtkObjectFactory)

set(Module_SRCS
  vtkOpenGLGPUVolumeRayCastMapper.cxx
  vtkOpenGLProjectedTetrahedraMapper.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkSmartVolumeMapper.cxx
  )

set_source_files_properties(
  vtkVolumeShader
  WRAP_EXCLUDE
  )

set(shader_files
  shaders/raycasterfs.glsl
  shaders/raycastervs.glsl
  shaders/vtkglProjectedTetrahedraVS.glsl
  shaders/vtkglProjectedTetrahedraFS.glsl
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
      --build-header VTKRENDERINGVOLUMEOPENGL2_EXPORT
      vtkRenderingVolumeOpenGL2Module.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()

# Now for the object factory.
set(opengl_overrides
  GPUVolumeRayCastMapper
  ProjectedTetrahedraMapper
  RayCastImageDisplayHelper
  )

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkRenderingVolumeOpenGL2 ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkRenderingVolumeOpenGL2)
