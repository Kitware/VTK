# Forbid downloading resources from the network during a build. This helps
# when building on systems without network connectivity to determine which
# resources much be obtained manually and made available to the build.
option(VTK_FORBID_DOWNLOADS "Do not download source code or data from the network" OFF)
mark_as_advanced(VTK_FORBID_DOWNLOADS)
macro(vtk_download_attempt_check _name)
  if(VTK_FORBID_DOWNLOADS)
    message(SEND_ERROR "Attempted to download ${_name} when VTK_FORBID_DOWNLOADS is ON")
  endif()
endmacro()
