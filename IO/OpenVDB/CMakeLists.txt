# Resolve the OpenVDB external dependency
find_path(OPENVDB_CMAKE_PATH
  FindOpenVDB.cmake
  DOC "Path to OpenVDB CMake files"
)

list(APPEND CMAKE_MODULE_PATH "${OPENVDB_CMAKE_PATH}")
vtk_module_find_package(PACKAGE OpenVDB)

set(classes
  vtkOpenVDBWriter)

vtk_module_add_module(VTK::IOOpenVDB
  CLASSES ${classes})
vtk_module_link(VTK::IOOpenVDB
  PRIVATE
    OpenVDB::openvdb)
