find_path(OpenXRRemoting_INCLUDE_DIR
  NAMES
    openxr_msft_holographic_remoting.h
    openxr_msft_remoting_frame_mirroring.h
    openxr_msft_remoting_speech.h
  DOC "OpenXR Remoting include directory")
mark_as_advanced(OpenXRRemoting_INCLUDE_DIR)

find_path(OpenXRRemoting_BIN_DIR
  NAMES
    Microsoft.Holographic.AppRemoting.OpenXr.dll
    RemotingXR.json
  DOC "OpenXR Remoting bin directory")
mark_as_advanced(OpenXRRemoting_BIN_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenXRRemoting
REQUIRED_VARS OpenXRRemoting_BIN_DIR OpenXRRemoting_INCLUDE_DIR)

if (OpenXRRemoting_FOUND)
  set(OpenXRRemoting_BINARIES
    ${OpenXRRemoting_BIN_DIR}/Microsoft.Holographic.AppRemoting.OpenXr.dll
    ${OpenXRRemoting_BIN_DIR}/PerceptionDevice.dll
    ${OpenXRRemoting_BIN_DIR}/RemotingXR.json)

  set(OpenXRRemoting_INCLUDE_DIRS "${OpenXRRemoting_INCLUDE_DIR}")

  if (NOT TARGET OpenXR::OpenXRRemoting)
    add_library(OpenXR::OpenXRRemoting INTERFACE IMPORTED)
    set_target_properties(OpenXR::OpenXRRemoting PROPERTIES
      IMPORTED_LOCATION "${OpenXRRemoting_BIN_DIR}/Microsoft.Holographic.AppRemoting.OpenXr.dll"
      INTERFACE_INCLUDE_DIRECTORIES "${OpenXRRemoting_INCLUDE_DIR}")
  endif ()
endif ()
