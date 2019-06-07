set(HDF5_EXTERNALLY_CONFIGURED 1)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkhdf5/COPYING"
                  "vtkhdf5/COPYING_LBNL_HDF5"
    VERSION       "1.10.5"
    STANDARD_INCLUDE_DIRS
    INTERFACE
  EXTERNAL
    PACKAGE     HDF5
    COMPONENTS  C HL
    TARGETS     hdf5::hdf5
                hdf5::hdf5_hl
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_hdf5.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")
