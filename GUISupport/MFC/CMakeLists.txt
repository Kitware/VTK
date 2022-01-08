# Detect CMP0091 state.
set(use_policy_0091 0)
if (POLICY CMP0091) # XXX(cmake-3.15)
  cmake_policy(GET CMP0091 policy_0091_state)
  if (policy_0091_state STREQUAL "NEW")
    set(use_policy_0091 1)
  endif ()
endif ()

# Determine whether the shared or static MFC implementation to use.
if (use_policy_0091) # XXX(cmake-3.15)
  if (NOT CMAKE_MSVC_RUNTIME_LIBRARY OR # The default uses `DLL`.
      CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL$")
    set(vtk_mfc_static 0)
  else ()
    set(vtk_mfc_static 1)
  endif ()
else ()
  # If "/MT" or "/MTd" is in the compiler flags, then our build
  # products will be linked to the C runtime static lib. Otherwise,
  # to the C runtime dll.
  #
  # Use the release flags.
  if (CMAKE_CXX_FLAGS_RELEASE MATCHES "/MT")
    set(vtk_mfc_static 1)
  else ()
    set(vtk_mfc_static 0)
  endif ()
endif ()

# C runtime lib linkage and MFC lib linkage *MUST* match.
# If linking to C runtime static lib, link to MFC static lib.
# If linking to C runtime dll, link to MFC dll.
if (vtk_mfc_static)
  set(CMAKE_MFC_FLAG 1)
else ()
  set(CMAKE_MFC_FLAG 2)
endif ()

set(classes
  vtkMFCWindow
  )
set(headers
  vtkMFCConfigure.h
  )

vtk_module_add_module(VTK::GUISupportMFC
  CLASSES ${classes}
  HEADERS ${headers})

if (vtk_mfc_static)
  vtk_module_link(VTK::GUISupportMFC
    PRIVATE
      "debug;nafxcwd;optimized;nafxcw"
      "debug;LIBCMTD;optimized;LIBCMT"
      Uxtheme
      windowscodecs)
else ()
  vtk_module_definitions(VTK::GUISupportMFC
    PRIVATE
      _AFXDLL)
endif ()
