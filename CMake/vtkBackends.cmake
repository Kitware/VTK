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

# Set a default backend type if none was specified, populate the enum.
if(NOT VTK_RENDERING_BACKEND)
  message(STATUS "Setting rendering backend to 'OpenGL' as none was specified.")
  set(VTK_RENDERING_BACKEND "OpenGL" CACHE STRING
    "Choose the rendering backend." FORCE)
  # Set the possible values of rendering backends for cmake-gui
  set_property(CACHE VTK_RENDERING_BACKEND PROPERTY
    STRINGS ${VTK_BACKENDS} "None")
endif()

# Now iterate through and enable the one that was selected.
foreach(backend ${VTK_BACKENDS})
  message(STATUS "Backend ${backend} modules: ${VTK_BACKEND_${backend}_MODULES}")
  if(${backend} STREQUAL "${VTK_RENDERING_BACKEND}")
    message(STATUS "Enabling modules for ${backend}.")
    foreach(module ${VTK_BACKEND_${backend}_MODULES})
      list(APPEND ${${module}_IMPLEMENTS}_IMPLEMENTATIONS ${module})
    endforeach()
  endif()
endforeach()
