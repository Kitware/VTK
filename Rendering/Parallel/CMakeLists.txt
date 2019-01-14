set(classes
  vtkClientServerCompositePass
  vtkClientServerSynchronizedRenderers
  vtkCompositedSynchronizedRenderers
  vtkCompositer
  vtkCompositeRenderManager
  vtkCompositeRGBAPass
  vtkCompositeZPass
  vtkCompressCompositer
  vtkImageRenderManager
  vtkParallelRenderManager
  vtkPHardwareSelector
  vtkSynchronizedRenderers
  vtkSynchronizedRenderWindows
  vtkTreeCompositer)

set(shader_files
  vtkCompositeZPassShader_fs.glsl
  vtkCompositeZPassFS.glsl)

set(sources)
set(private_headers)
foreach (shader_file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_SYMBOL "VTKRENDERINGPARALLEL_EXPORT"
    EXPORT_HEADER "vtkRenderingParallelModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND private_headers ${header})
endforeach ()

vtk_module_add_module(VTK::RenderingParallel
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

vtk_module_definitions(VTK::RenderingParallel
  PRIVATE
    VTK_OPENGL2)
