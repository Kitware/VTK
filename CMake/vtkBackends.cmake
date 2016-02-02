# Default code to handle VTK backends. The module.cmake files specify
# which backend the modules are in. We can specify some more specific
# documentation for backends in this file that will be displayed in
# cmake-gui and ccmake.
#
# The OpenGL backend is the current default, and the OpenGL2 backend is
# the new rendering code. This differs from groups in that only one backend
# can be built/linked to at any given time. The backend modules should use a
# naming convention where the backend name is the final word in the
# module name, i.e. vtkRenderingOpenGL for OpenGL and vtkRenderingOpenGL2
# for OpenGL2.

# is the current backend not a valid value?
set (_options ${VTK_BACKENDS} "None")
list (FIND _options "${VTK_RENDERING_BACKEND}"  _index)
if (${_index} EQUAL -1)

  # has the application defined a desired default for the backend?
  # if not, use VTKs default of OpenGL2
  if(NOT DEFINED VTK_RENDERING_BACKEND_DEFAULT)
    set(VTK_RENDERING_BACKEND_DEFAULT "OpenGL2")
  endif()

  # if it is in the cache as a bad value we need to reset it
  if(DEFINED VTK_RENDERING_BACKEND)
    message(STATUS "The cache contains an illegal value for VTK_RENDERING_BACKEND, forcing it to the default value of '${VTK_RENDERING_BACKEND_DEFAULT}'.")
    set(VTK_RENDERING_BACKEND "${VTK_RENDERING_BACKEND_DEFAULT}" CACHE STRING
        "Choose the rendering backend." FORCE)
  else()
    # otherwise just initialize it to the default determined above
    message(STATUS "Setting rendering backend to '${VTK_RENDERING_BACKEND_DEFAULT}' as none was specified.")
    set(VTK_RENDERING_BACKEND "${VTK_RENDERING_BACKEND_DEFAULT}" CACHE STRING
      "Choose the rendering backend.")
  endif()

  # Set the possible values of rendering backends for cmake-gui
  set_property(CACHE VTK_RENDERING_BACKEND PROPERTY
    STRINGS ${_options})
endif()

# Now iterate through and enable the one that was selected.
foreach(backend ${VTK_BACKENDS})
  message(STATUS "Backend ${backend} modules: ${VTK_BACKEND_${backend}_MODULES}")
  if(${backend} STREQUAL "${VTK_RENDERING_BACKEND}")
    message(STATUS "Enabling modules for ${backend}.")
    foreach(module ${VTK_BACKEND_${backend}_MODULES})
      if (${${module}_IMPLEMENTATION_REQUIRED_BY_BACKEND})
        list(APPEND ${${module}_IMPLEMENTS}_IMPLEMENTATIONS ${module})
      endif()
    endforeach()
  endif()
endforeach()

# check for None with rendering turned on
if(VTK_RENDERING_BACKEND STREQUAL "None" AND VTK_Group_Rendering)
  message(FATAL_ERROR "VTK_Group_Rendering is on when the rendering backend is set to None. Please either turn off the rendering group or set the rendering backend to a different value")
endif()

if (VTK_RENDERING_BACKEND STREQUAL "None")
  # with no backend make a dummy None modules
  vtk_module(vtkRenderingNone )
  vtk_module(vtkRenderingContextNone )
  vtk_module(vtkRenderingVolumeNone )
  vtk_module(vtkIOExportNone ) # GL2PSExporter differs on OGL backends
endif()
