add_subdirectory(Ex1)
if (TARGET VTK::RenderingOpenGL2)
  add_subdirectory(Ex2)
endif ()
