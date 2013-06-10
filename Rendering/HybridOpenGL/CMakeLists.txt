set(Module_SRCS
  vtkImageDataLIC2D.cxx
  vtkImageDataLIC2DExtentTranslator.cxx
  vtkStructuredGridLIC2D.cxx
  vtkSurfaceLICDefaultPainter.cxx
  vtkSurfaceLICPainter.cxx
  )

set(shader_files
  vtkStructuredGridLIC2D_fs.glsl
  vtkSurfaceLICPainter_fs1.glsl
  vtkSurfaceLICPainter_fs2.glsl
  vtkSurfaceLICPainter_vs1.glsl
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
      --build-header VTKRENDERINGHYBRIDOPENGL_EXPORT vtkRenderingHybridOpenGLModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

vtk_module_library(${vtk-module} ${Module_SRCS})
