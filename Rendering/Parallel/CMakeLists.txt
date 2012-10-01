set(Module_SRCS
  vtkClientServerCompositePass.cxx
  vtkClientServerSynchronizedRenderers.cxx
  vtkCompositedSynchronizedRenderers.cxx
  vtkCompositer.cxx
  vtkCompositeRenderManager.cxx
  vtkCompositeRGBAPass.cxx
  vtkCompositeZPass.cxx
  vtkCompressCompositer.cxx
  vtkParallelRenderManager.cxx
  vtkPHardwareSelector.cxx
  vtkSynchronizedRenderers.cxx
  vtkSynchronizedRenderWindows.cxx
  vtkTreeCompositer.cxx
  vtkImageRenderManager.cxx
  )

set(shader_files vtkCompositeZPassShader_fs.glsl)

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
      --build-header VTKRENDERINGPARALLEL_EXPORT vtkRenderingParallelModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

set_source_files_properties(
  vtkParallelRenderManager
  ABSTRACT
  )

set_source_files_properties(
  vtkCompositeRGBAPass
  vtkCompositeZPass
  WRAP_EXCLUDE
  )

vtk_module_library(vtkRenderingParallel ${Module_SRCS})
