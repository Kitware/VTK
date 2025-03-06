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
  vtkSynchronizableActors
  vtkSynchronizableAvatars
  vtkSynchronizedRenderers
  vtkSynchronizedRenderWindows
  vtkTreeCompositer)

set(shader_files
  vtkCompositeZPassShader_fs.glsl)
set(public_shader_files
  vtkCompositeZPassFS.glsl)

set(sources)
set(private_headers)
foreach (shader_file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_HEADER "vtkRenderingParallelModule.h"
    EXPORT_SYMBOL "VTKRENDERINGPARALLEL_NO_EXPORT"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND private_headers ${header})
endforeach ()
set(headers)
foreach (shader_file IN LISTS public_shader_files)
  vtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_HEADER "vtkRenderingParallelModule.h"
    EXPORT_SYMBOL "VTKRENDERINGPARALLEL_EXPORT"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    ABI_MANGLE_HEADER       "vtkABINamespace.h"
    ABI_MANGLE_SYMBOL_BEGIN "VTK_ABI_NAMESPACE_BEGIN"
    ABI_MANGLE_SYMBOL_END   "VTK_ABI_NAMESPACE_END")
  list(APPEND sources ${source})
  list(APPEND headers ${header})
endforeach ()

vtk_module_add_module(VTK::RenderingParallel
  CLASSES ${classes}
  SOURCES ${sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

vtk_module_definitions(VTK::RenderingParallel
  PRIVATE
    VTK_OPENGL2)
vtk_add_test_mangling(VTK::RenderingParallel)
