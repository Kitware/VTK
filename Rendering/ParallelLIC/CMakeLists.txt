include(vtkObjectFactory)

set(Module_SRCS
  vtkMPIPixelTT.cxx
  vtkParallelTimer.cxx
  vtkPPixelTransfer.cxx
  vtkPLineIntegralConvolution2D.cxx
  vtkPPainterCommunicator.cxx
  vtkPSurfaceLICComposite.cxx
  vtkPSurfaceLICInterface.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingParallelLICObjectFactory.cxx
  )

option(
  VTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER
  "enable parallel timers for the surface lic painter"
  OFF
  )
mark_as_advanced(VTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
option(
  VTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVLOLUTION2D_TIMER
  "enable parallel timers for the 2d line integral convolution"
  OFF
  )
mark_as_advanced(VTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVLOLUTION2D_TIMER)
if (VTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
  add_definitions("-DvtkSurfaceLICPainterTIME")
endif()
if (VTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVLOLUTION2D_TIMER)
  add_definitions("-DvtkLineIntegralConvolution2DTIME")
endif()

vtk_add_override(vtkSurfaceLICInterface vtkPSurfaceLICInterface)
set(shader_files
  vtkPSurfaceLICComposite_CompFS.glsl
  )

unset(shader_h_files)
foreach(file ${shader_files})
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGPARALLELLIC_EXPORT"
    EXPORT_HEADER "vtkRenderingParallelLICModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
endforeach()

vtk_add_override(vtkLineIntegralConvolution2D vtkPLineIntegralConvolution2D)
vtk_add_override(vtkSurfaceLICComposite vtkPSurfaceLICComposite)
vtk_object_factory_configure("${vtk_module_overrides}")

include(vtkMPI)

vtk_module_library(${vtk-module} ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

vtk_mpi_link(${vtk-module})
