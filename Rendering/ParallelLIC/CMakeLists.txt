set(Module_SRCS
  vtkMPIPixelTT.cxx
  vtkParallelTimer.cxx
  vtkPPixelTransfer.cxx
  vtkPLineIntegralConvolution2D.cxx
  vtkPPainterCommunicator.cxx
  vtkPSurfaceLICComposite.cxx
  vtkPSurfaceLICPainter.cxx
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

set(shader_files
  vtkPSurfaceLICComposite_Comp.glsl
  )

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
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

# Now we need to generate the object factory for this module. Create a list of
# overrides, and then generate the class that overrides those classes.
macro(vtk_add_override base override)
  list(APPEND vtk_module_overrides ${base})
  set(vtk_module_${base}_override ${override})
endmacro()
vtk_add_override(vtkLineIntegralConvolution2D vtkPLineIntegralConvolution2D)
vtk_add_override(vtkSurfaceLICComposite vtkPSurfaceLICComposite)
vtk_add_override(vtkSurfaceLICPainter vtkPSurfaceLICPainter)

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(
  ${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h
  )
configure_file(
  ${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_mpi_link(${vtk-module})

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${MPI_C_INCLUDE_PATH})
