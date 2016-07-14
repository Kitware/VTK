include(vtkObjectFactory)

set(Module_SRCS
  vtkMPIPixelTT.cxx
  vtkParallelTimer.cxx
  vtkPPixelTransfer.cxx
  vtkPLineIntegralConvolution2D.cxx
  vtkPPainterCommunicator.cxx
  vtkPSurfaceLICComposite.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingParallelLICObjectFactory.cxx
  )

set_source_files_properties(
  vtkMPIPixelTT.cxx
  vtkPPixelTransfer.cxx
  vtkPLineIntegralConvolution2D.cxx
  vtkPPainterCommunicator.cxx
  vtkPSurfaceLICComposite.cxx
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkMPIPixelTT.cxx
  vtkPPixelTransfer.cxx
  vtkPSurfaceLICComposite.cxx
  PROPERTIES WRAP_EXCLUDE_PYTHON 1
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

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  list(APPEND Module_SRCS
    vtkPSurfaceLICInterface.cxx
    )
  vtk_add_override(vtkSurfaceLICInterface vtkPSurfaceLICInterface)
  set(shader_files
    vtkPSurfaceLICComposite_CompFS.glsl
    )
else()
  list(APPEND Module_SRCS
    vtkPSurfaceLICPainter.cxx
    )
  vtk_add_override(vtkSurfaceLICPainter vtkPSurfaceLICPainter)
  set(shader_files
    vtkPSurfaceLICComposite_Comp.glsl
    )
endif()

unset(shader_h_files)
foreach(file ${shader_files})
  get_filename_component(file_we ${file} NAME_WE)
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx)
  set(resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h)
  list(APPEND shader_h_files ${resh})
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file_we}
    --build-header
    VTKRENDERINGPARALLELLIC_EXPORT
    vtkRenderingParallelLICModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()

vtk_add_override(vtkLineIntegralConvolution2D vtkPLineIntegralConvolution2D)
vtk_add_override(vtkSurfaceLICComposite vtkPSurfaceLICComposite)
vtk_object_factory_configure("${vtk_module_overrides}")

include(vtkMPI)

vtk_module_library(${vtk-module} ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

vtk_mpi_link(${vtk-module})
