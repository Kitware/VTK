set(PACKAGE_NAME "h5hut")
set(PACKAGE_VERSION "2.0.0rc7")
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
set(PACKAGE_TARNAME "${PROJECT_NAME}")
set(PACKAGE_BUGREPORT "h5part@lists.psi.ch")
set(PACKAGE_URL "")

set(USE_LARGE_INDICES TRUE)
set(H5_USE_LUSTRE FALSE)
set(WITH_PARALLEL_H5GRID FALSE)

set(headers
  src/include/h5core/h5_attachments.h
  src/include/h5core/h5_err.h
  src/include/h5core/h5_file.h
  src/include/h5core/h5_file_attribs.h
  src/include/h5core/h5_log.h
  src/include/h5core/h5_model.h
  src/include/h5core/h5_step_attribs.h
  src/include/h5core/h5_syscall.h
  src/include/h5core/h5_types.h
  src/include/h5core/h5b_attribs.h
  src/include/h5core/h5b_io.h
  src/include/h5core/h5b_model.h
  src/include/h5core/h5t_adjacencies.h
  src/include/h5core/h5t_map.h
  src/include/h5core/h5t_model.h
  src/include/h5core/h5t_octree.h
  src/include/h5core/h5t_retrieve.h
  src/include/h5core/h5t_store.h
  src/include/h5core/h5t_tags.h
  src/include/h5core/h5u_io.h
  src/include/h5core/h5u_model.h

  src/include/H5_attachments.h
  src/include/H5_err.h
  src/include/H5_file.h
  src/include/H5_file_attribs.h
  src/include/H5_log.h
  src/include/H5_model.h
  src/include/H5_step_attribs.h
  src/include/H5Block_attribs.h
  src/include/H5Block_io.h
  src/include/H5Block_model.h
  src/include/H5Fed_adjacency.h
  src/include/H5Fed_model.h
  src/include/H5Fed_retrieve.h
  src/include/H5Fed_store.h
  src/include/H5Fed_tags.h
  src/include/H5hut.h
  src/include/H5Part_io.h
  src/include/H5Part_model.h
  src/include/vtk_h5hut_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/src/include/vtkh5hut_export.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/include/config.h")

set(sources
  src/h5core/h5_attachments.c
  src/h5core/h5_attribs.c
  src/h5core/h5_err.c
  src/h5core/h5_file.c
  src/h5core/h5_init.c
  src/h5core/h5_log.c
  src/h5core/h5_model.c
  src/h5core/h5_syscall.c
  src/h5core/h5b_attribs.c
  src/h5core/h5b_io.c
  src/h5core/h5b_model.c
  src/h5core/h5t_adjacencies.c
  src/h5core/h5t_io.c
  src/h5core/h5t_map.c
  src/h5core/h5t_model.c
  src/h5core/h5t_octree.c
  src/h5core/h5t_retrieve.c
  src/h5core/h5t_store.c
  src/h5core/h5t_tags.c
  src/h5core/h5u_io.c
  src/h5core/h5u_model.c)

set(private_headers
  src/h5core/private/h5_attribs.h
  src/h5core/private/h5_const.h
  src/h5core/private/h5_err.h
  src/h5core/private/h5_fcmp.h
  src/h5core/private/h5_file.h
  src/h5core/private/h5_hdf5.h
  src/h5core/private/h5_hsearch.h
  src/h5core/private/h5_init.h
  src/h5core/private/h5_io.h
  src/h5core/private/h5_log.h
  src/h5core/private/h5_lustre.h
  src/h5core/private/h5_maps.h
  src/h5core/private/h5_model.h
  src/h5core/private/h5_mpi.h
  src/h5core/private/h5_qsort.h
  src/h5core/private/h5_types.h
  src/h5core/private/h5_va_macros.h
  src/h5core/private/h5b_io.h
  src/h5core/private/h5b_model.h
  src/h5core/private/h5b_types.h
  src/h5core/private/h5t_access.h
  src/h5core/private/h5t_adjacencies.h
  src/h5core/private/h5t_core.h
  src/h5core/private/h5t_err.h
  src/h5core/private/h5t_io.h
  src/h5core/private/h5t_map.h
  src/h5core/private/h5t_model.h
  src/h5core/private/h5t_octree.h
  src/h5core/private/h5t_ref_elements.h
  src/h5core/private/h5t_retrieve.h
  src/h5core/private/h5t_store.h
  src/h5core/private/h5t_tags.h
  src/h5core/private/h5t_types.h
  src/h5core/private/h5u_err.h
  src/h5core/private/h5u_io.h
  src/h5core/private/h5u_types.h)

set(private_sources
  src/h5core/private/h5_fcmp.c
  src/h5core/private/h5_hdf5.c
  src/h5core/private/h5_hsearch.c
  src/h5core/private/h5_io.c
  src/h5core/private/h5_lustre.c
  src/h5core/private/h5_maps.c
  src/h5core/private/h5_qsort.c
  src/h5core/private/h5_qsort_r.c
  src/h5core/private/h5t_access_tetm.c
  src/h5core/private/h5t_access_trim.c
  src/h5core/private/h5t_adjacencies_tetm.c
  src/h5core/private/h5t_adjacencies_trim.c
  src/h5core/private/h5t_core.c
  src/h5core/private/h5t_core_tetm.c
  src/h5core/private/h5t_core_trim.c
  src/h5core/private/h5t_io_tetm.c
  src/h5core/private/h5t_io_trim.c
  src/h5core/private/h5t_model_tetm.c
  src/h5core/private/h5t_model_trim.c
  src/h5core/private/h5t_ref_elements.c
  src/h5core/private/h5t_retrieve_tetm.c
  src/h5core/private/h5t_retrieve_trim.c
  src/h5core/private/h5t_store_tetm.c
  src/h5core/private/h5t_store_trim.c)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/include/config.h)

vtk_module_add_module(VTK::h5hut
  HEADER_DIRECTORIES
  SOURCES ${sources} ${private_sources}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  HEADERS_SUBDIR "vtkh5hut")
vtk_module_include(VTK::h5hut
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/include>"
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/h5core>"
  INTERFACE
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkh5hut/src/include>")

include(GenerateExportHeader)
generate_export_header(vtkh5hut
  EXPORT_MACRO_NAME VTKH5HUT_EXPORT
  EXPORT_FILE_NAME src/include/vtkh5hut_export.h)
