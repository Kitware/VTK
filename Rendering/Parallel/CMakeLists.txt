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

set(shader_files vtkCompositeZPassShader_fs.glsl vtkCompositeZPassFS.glsl)

unset(shader_h_files)
foreach(file ${shader_files})
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGPARALLEL_EXPORT"
    EXPORT_HEADER "vtkRenderingParallelModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
  set_source_files_properties(
    ${header}
    ${source}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()

set_source_files_properties(
  vtkCompositeRGBAPass
  vtkCompositeZPass
  WRAP_EXCLUDE
  )

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTK_OPENGL2)
endif()

vtk_module_library(vtkRenderingParallel ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkRenderingParallel)
