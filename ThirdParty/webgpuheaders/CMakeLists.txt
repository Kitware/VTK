vtk_module_third_party_internal(
  LICENSE_FILES "vtkwebgpuheaders/LICENSE"
  SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT "Copyright 2024 The Dawn & Tint Authors"
  SPDX_DOWNLOAD_LOCATION "git+https://dawn.googlesource.com/dawn"
  VERSION "20260720.160313"
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
