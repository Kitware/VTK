find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkClientServerCompositePass.cxx
  vtkClientServerSynchronizedRenderers.cxx
  vtkCompositedSynchronizedRenderers.cxx
  vtkCompositer.cxx
  vtkCompositeRenderManager.cxx
  vtkCompositeRGBAPass.cxx
  # vtkCompositeZPass.cxx # fixme shaders
  #  vtkCompositeZPassShader_fs.glsl
  vtkCompressCompositer.cxx
  vtkParallelRenderManager.cxx
  vtkPHardwareSelector.cxx
  vtkSynchronizedRenderers.cxx
  vtkSynchronizedRenderWindows.cxx
  vtkTreeCompositer.cxx
  vtkImageRenderManager.cxx
  )

set_source_files_properties(
  vtkParallelRenderManager
  ABSTRACT
  )

set_source_files_properties(
  vtkCompositeRGBAPass
  WRAP_EXCLUDE
  )

vtk_module_library(vtkRenderingParallel ${Module_SRCS})
