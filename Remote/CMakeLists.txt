# Functions to fetch remote modules.
include(vtkModuleRemote)

file(GLOB remotes "*.remote.cmake")
foreach(remote_module ${remotes})
  include(${remote_module})
endforeach()
