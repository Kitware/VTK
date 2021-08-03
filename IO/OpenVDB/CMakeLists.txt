# Resolve the OpenVDB external dependency
find_path(OPENVDB_CMAKE_PATH
  FindOpenVDB.cmake
  DOC "Path to OpenVDB CMake files"
)

list(APPEND CMAKE_MODULE_PATH "${OPENVDB_CMAKE_PATH}")
find_package(OpenVDB REQUIRED)

set(classes
  vtkOpenVDBWriter)

vtk_module_add_module(VTK::OpenVDB
  CLASSES ${classes})
vtk_module_link(VTK::OpenVDB
  PRIVATE
    OpenVDB::openvdb)
