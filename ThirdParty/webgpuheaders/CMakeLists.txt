vtk_module_third_party_internal(
  LICENSE_FILES "vtkwebgpuheaders/LICENSE"
  SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT "Copyright (c) 2019, \"WebGPU native\" developers"
  SPDX_DOWNLOAD_LOCATION "git+https://github.com/webgpu-native/webgpu-headers"
  VERSION "b3f67b89929c133403fd95638be4ef96b56ddca0"
  HEADER_ONLY)

# The module is header only, so its target is an INTERFACE library, and the SPDX
# file is written by a custom target hung off it. Nothing builds an INTERFACE
# library on its own: the custom target only runs because a module that links
# these headers pulls it in. In a build where no module does - anything without
# the WebGPU backend - the file is never written and the install step then fails
# looking for it. Ask for it as part of the default build instead.
if (TARGET vtkwebgpuheaders-spdx)
  set_property(TARGET vtkwebgpuheaders-spdx PROPERTY EXCLUDE_FROM_ALL OFF)
endif ()
