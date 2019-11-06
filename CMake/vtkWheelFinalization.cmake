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

file(WRITE "${CMAKE_BINARY_DIR}/vtk_features.py"
  "FEATURES = {\n${vtk_feature_entries}}\n")
