set(vtk_feature_entries "")

if (TARGET VTK::mpi)
  string(APPEND vtk_feature_entries
    "    'mpi': [],\n")
endif ()

if (TARGET VTK::RenderingCore)
  string(APPEND vtk_feature_entries
    "    'rendering': [],\n")
endif ()

if (TARGET VTK::RenderingOpenVR)
  string(APPEND vtk_feature_entries
    "    'openvr': [],\n")
endif ()

if (TARGET VTK::RenderingOpenGL2)
  set(has_onscreen OFF)
  set(has_offscreen OFF)
  if (VTK_USE_X)
    string(APPEND vtk_feature_entries
      "    'rendering-onscreen-x11': [],\n")
    set(has_onscreen ON)
  endif ()
  if (VTK_USE_COCOA)
    string(APPEND vtk_feature_entries
      "    'rendering-onscreen-cocoa': [],\n")
    set(has_onscreen ON)
  endif ()
  if (WIN32)
    string(APPEND vtk_feature_entries
      "    'rendering-onscreen-windows': [],\n")
    set(has_onscreen ON)
  endif ()
  if (VTK_OPENGL_HAS_OSMESA)
    string(APPEND vtk_feature_entries
      "    'rendering-offscreen-osmesa': [],\n")
    set(has_offscreen ON)
  endif ()

  if (has_onscreen)
    string(APPEND vtk_feature_entries
      "    'rendering-onscreen': [],\n")
  endif ()
  if (has_offscreen)
    string(APPEND vtk_feature_entries
      "    'rendering-offscreen': [],\n")
  endif ()

  if (VTK_OPENGL_USE_GLES)
    string(APPEND vtk_feature_entries
      "    'rendering-backend-gles': [],\n")
  else ()
    string(APPEND vtk_feature_entries
      "    'rendering-backend-gl': [],\n")
  endif ()
  if (VTK_OPENGL_HAS_EGL)
    string(APPEND vtk_feature_entries
      "    'rendering-backend-egl': [],\n")
  endif ()
endif ()

file(WRITE "${CMAKE_BINARY_DIR}/vtk_features.py"
  "FEATURES = {\n${vtk_feature_entries}}\n")
