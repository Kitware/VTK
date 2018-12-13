# Functions to fetch remote modules.
include(vtkModuleRemote)

file(GLOB remotes "*.remote.cmake")
foreach(remote_module ${remotes})
  if (remote_module MATCHES "/\\.")
    continue ()
  endif ()
  include(${remote_module})
endforeach()
