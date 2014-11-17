if(TARGET vtkRenderingOpenGL)
  message(FATAL_ERROR "vtkChemistyOpenGL2 cannot be built with vtkRenderingOpenGL, please disable one of them.")
endif()

include(vtkObjectFactory)

set(Module_SRCS
  vtkOpenGLMoleculeMapper.cxx
  vtkOpenGLSphereMapper.cxx
  vtkOpenGLStickMapper.cxx
  )

set(shader_files
  glsl/vtkSphereMapperVS.glsl
  glsl/vtkStickMapperVS.glsl
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
      --build-header VTKDOMAINSCHEMISTRYOPENGL2_EXPORT
      vtkDomainsChemistryOpenGL2Module.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

# Now for the object factory.
set(opengl_overrides
  MoleculeMapper
  )

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(${vtk-module}ObjectFactory.cxx WRAP_EXCLUDE)

vtk_module_library(vtkDomainsChemistryOpenGL2 ${Module_SRCS})
