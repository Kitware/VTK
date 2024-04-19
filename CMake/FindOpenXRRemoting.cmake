find_path(OpenXRRemoting_INCLUDE_DIR
  NAMES
    openxr_msft_holographic_remoting.h
    openxr_msft_remoting_frame_mirroring.h
    openxr_msft_remoting_speech.h
  PATH_SUFFIXES
    openxr
    openxrremoting
  DOC "OpenXR Remoting include directory")
mark_as_advanced(OpenXRRemoting_INCLUDE_DIR)

find_path(OpenXRRemoting_BIN_DIR
  NAMES
    Microsoft.Holographic.AppRemoting.OpenXr.dll
    RemotingXR.json
  PATH_SUFFIXES
    "x64/Desktop"
    "bin/x64/Desktop"
  DOC "OpenXR Remoting bin directory")
mark_as_advanced(OpenXRRemoting_BIN_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenXRRemoting
  REQUIRED_VARS OpenXRRemoting_INCLUDE_DIR OpenXRRemoting_BIN_DIR)

if (OpenXRRemoting_FOUND)

  # OpenXR remoting headers
  if (NOT TARGET OpenXR::Remoting)
    add_library(OpenXR::Remoting INTERFACE IMPORTED)
    set_target_properties(OpenXR::Remoting PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${OpenXRRemoting_INCLUDE_DIR}")
  endif()

  # OpenXR remoting runtime library
  if (NOT TARGET OpenXR::RemotingRuntime)
    add_library(OpenXR::RemotingRuntime MODULE IMPORTED)
    set_target_properties(OpenXR::RemotingRuntime PROPERTIES
      IMPORTED_LOCATION "${OpenXRRemoting_BIN_DIR}/Microsoft.Holographic.AppRemoting.OpenXr.dll")
  endif()

  # OpenXR remoting PerceptionDevice library
  if (NOT TARGET OpenXR::PerceptionDevice)
    add_library(OpenXR::PerceptionDevice MODULE IMPORTED)
    set_target_properties(OpenXR::PerceptionDevice PROPERTIES
      IMPORTED_LOCATION "${OpenXRRemoting_BIN_DIR}/PerceptionDevice.dll")
  endif()

  # RemotingXR.json
  set(RemotingXR_JSON "${OpenXRRemoting_BIN_DIR}/RemotingXR.json")

endif()
