# Only test as part of VTK itself.
if (NOT CMAKE_PROJECT_NAME STREQUAL "VTK")
  return ()
endif ()

add_subdirectory(Cxx)
