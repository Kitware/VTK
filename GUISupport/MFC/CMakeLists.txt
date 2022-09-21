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
vtk_add_test_mangling(VTK::GUISupportMFC)

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

# Using the linker /DELAYLOAD flag is necessary when VTK is built as dlls to
# avoid false mem leak reporting by MFC shutdown code. Without it, the VTK dlls
# load before the MFC dll at startup and unload after the MFC dll unloads at
# shutdown. Hence, any VTK objects left at MFC dll unload time get reported as
# leaks.
if (BUILD_SHARED_LIBS)
  vtk_module_link(VTK::GUISupportMFC
    PRIVATE
      DelayImp)

  # Direct dependencies.
  set(mfc_dependencies
    # Public
    VTK::RenderingOpenGL2)
  set(mfc_dependencies_visited)

  set(mfc_library_dependencies)
  while (mfc_dependencies)
    list(GET mfc_dependencies 0 mfc_dependency)
    list(REMOVE_AT mfc_dependencies 0)
    if (mfc_dependency IN_LIST mfc_dependencies_visited)
      continue ()
    endif ()
    list(APPEND mfc_dependencies_visited
      "${mfc_dependency}")

    _vtk_module_get_module_property("${mfc_dependency}"
      PROPERTY  kit
      VARIABLE  mfc_dependency_kit)
    if (mfc_dependency_kit)
      list(APPEND mfc_library_dependencies
        "${mfc_dependency_kit}")
    else ()
      vtk_module_get_property("${mfc_dependency}"
        PROPERTY  TYPE
        VARIABLE  mfc_dependency_type)
      if (mfc_dependency_type STREQUAL "SHARED_LIBRARY")
        list(APPEND mfc_library_dependencies
          "${mfc_dependency}")
      endif ()
    endif ()

    _vtk_module_get_module_property("${mfc_dependency}"
      PROPERTY  depends
      VARIABLE  mfc_dependency_dependencies)
    list(APPEND mfc_dependencies
      ${mfc_dependency_dependencies})
  endwhile ()

  if (mfc_library_dependencies) # XXX(cmake-3.14): `REMOVE_DUPLICATES` on an empty list
    list(REMOVE_DUPLICATES mfc_dependency_kit)
  endif ()

  set(vtk_mfc_link_options)
  foreach (mfc_library_dependency IN LISTS mfc_library_dependencies)
    list(APPEND vtk_mfc_link_options
      "/DELAYLOAD:$<TARGET_FILE_NAME:${mfc_library_dependency}>")
  endforeach ()

  if (NOT CMAKE_VERSION VERSION_LESS "3.13") # XXX(cmake-3.13): target_link_options.
    vtk_module_link_options(VTK::GUISupportMFC
      PRIVATE
        ${vtk_mfc_link_options})
  else ()
    vtk_module_link(VTK::GUISupportMFC
      PRIVATE
        ${vtk_mfc_link_options})
  endif ()
endif ()
