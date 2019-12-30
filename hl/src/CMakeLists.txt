if (FALSE) # XXX(kitware): cmake versions are handled by VTK.
cmake_minimum_required (VERSION 3.10)
endif ()
project (HDF5_HL_SRC C)

#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------
set (HL_SOURCES
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DO.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DS.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IM.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LT.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTanalyze.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PT.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TB.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LD.c
)

set (HL_HEADERS
    ${HL_HEADERS}
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DOpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DSpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IMpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TBpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LDpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/hdf5_hl.h
    ${HDF5_HL_SRC_SOURCE_DIR}/vtk_hdf5_hl_mangle.h
)

set (HL_PRIVATE_HEADERS
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.h
)

if (FALSE) # XXX(kitware): Use VTK's module system.
if (NOT ONLY_SHARED_LIBS)
  add_library (${HDF5_HL_LIB_TARGET} STATIC ${HL_SOURCES} ${HL_HEADERS} ${HL_PRIVATE_HEADERS})
  target_include_directories (${HDF5_HL_LIB_TARGET}
      PRIVATE "${HDF5_SRC_DIR};${HDF5_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  TARGET_C_PROPERTIES (${HDF5_HL_LIB_TARGET} STATIC)
  target_link_libraries (${HDF5_HL_LIB_TARGET} PUBLIC ${HDF5_LIB_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_HL_LIB_TARGET} ${HDF5_HL_LIB_NAME} STATIC 0)
  set_target_properties (${HDF5_HL_LIB_TARGET} PROPERTIES FOLDER libraries/hl)
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIB_TARGET}")
  set (install_targets ${HDF5_HL_LIB_TARGET})
endif ()

else ()
  add_library(vtkhdf5_hl_src ${HL_SOURCES} ${HL_HEADERS} ${HL_PRIVATE_HEADERS})
target_include_directories(vtkhdf5_hl_src
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HDF5_BINARY_DIR}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkhdf5/hl/src>")
vtk_module_install_headers(
  FILES   ${HL_HEADERS}
  SUBDIR  "vtkhdf5/hl/src")
if (BUILD_SHARED_LIBS)
  target_compile_definitions(vtkhdf5_hl_src
    PUBLIC
      H5_BUILT_AS_DYNAMIC_LIB
    PRIVATE
      hdf5_hl_shared_EXPORTS)
endif ()
target_link_libraries(vtkhdf5_hl_src
  PRIVATE
    vtkhdf5_src)
_vtk_module_apply_properties(vtkhdf5_hl_src)
_vtk_module_install(vtkhdf5_hl_src)

set(_vtk_add_module_output_name "vtkhdf5_hl")
if (_vtk_build_LIBRARY_NAME_SUFFIX)
  string(APPEND _vtk_add_module_output_name "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
endif ()

set_target_properties(vtkhdf5_hl_src
  PROPERTIES
    OUTPUT_NAME "${_vtk_add_module_output_name}")
endif ()

if (BUILD_SHARED_LIBS AND FALSE) # XXX(kitware): VTK handles shared/static flags.
  add_library (${HDF5_HL_LIBSH_TARGET} SHARED ${HL_SOURCES} ${HL_HEADERS} ${HL_PRIVATE_HEADERS})
  target_include_directories (${HDF5_HL_LIBSH_TARGET}
      PRIVATE "${HDF5_SRC_DIR};${HDF5_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_definitions(${HDF5_HL_LIBSH_TARGET}
      PUBLIC "H5_BUILT_AS_DYNAMIC_LIB"
  )
  TARGET_C_PROPERTIES (${HDF5_HL_LIBSH_TARGET} SHARED)
  target_link_libraries (${HDF5_HL_LIBSH_TARGET} PUBLIC ${HDF5_LIBSH_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_HL_LIBSH_TARGET} ${HDF5_HL_LIB_NAME} SHARED "HL")
  set_target_properties (${HDF5_HL_LIBSH_TARGET} PROPERTIES FOLDER libraries/hl)
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIBSH_TARGET}")
  set (install_targets ${install_targets} ${HDF5_HL_LIBSH_TARGET})
endif ()

if (FALSE) # XXX(kitware): VTK handles installation.
#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install (
    FILES
        ${HL_HEADERS}
    DESTINATION
        ${HDF5_INSTALL_INCLUDE_DIR}
    COMPONENT
        hlheaders
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_LIBSH_TARGET} ${HDF5_INSTALL_BIN_DIR} hllibraries)
  endif ()
  if (NOT ONLY_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} hllibraries)
  endif ()

  install (
      TARGETS
          ${install_targets}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hllibraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hllibraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT hllibraries
      FRAMEWORK DESTINATION ${HDF5_INSTALL_FWRK_DIR} COMPONENT hllibraries
      INCLUDES DESTINATION include
  )
endif ()

#-----------------------------------------------------------------------------
# Create pkgconfig files
#-----------------------------------------------------------------------------
set (_PKG_CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX})
set (_PKG_CONFIG_EXEC_PREFIX \${prefix})
set (_PKG_CONFIG_LIBDIR \${exec_prefix}/lib)
set (_PKG_CONFIG_INCLUDEDIR \${prefix}/include)
set (_PKG_CONFIG_LIBNAME "${HDF5_HL_LIB_CORENAME}")
set (_PKG_CONFIG_VERSION "${HDF5_PACKAGE_VERSION}")

set (_PKG_CONFIG_LIBS_PRIVATE)

if (NOT ONLY_SHARED_LIBS)
  set (_PKG_CONFIG_LIBS "${_PKG_CONFIG_LIBS} -l${HDF5_HL_LIB_CORENAME}")
endif ()
if (BUILD_SHARED_LIBS)
  set (_PKG_CONFIG_SH_LIBS "${_PKG_CONFIG_SH_LIBS} -l${HDF5_HL_LIB_CORENAME}")
endif ()

set (_PKG_CONFIG_REQUIRES "${HDF5_LIB_CORENAME}")
set (_PKG_CONFIG_REQUIRES_PRIVATE "${HDF5_LIB_CORENAME}")

configure_file (
    ${HDF_RESOURCES_DIR}/libhdf5.pc.in
    ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_HL_LIB_CORENAME}-${HDF5_PACKAGE_VERSION}.pc
    @ONLY
)
install (
    FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_HL_LIB_CORENAME}-${HDF5_PACKAGE_VERSION}.pc
    DESTINATION ${HDF5_INSTALL_LIB_DIR}/pkgconfig
    COMPONENT hllibraries
)

if (NOT WIN32 AND NOT MINGW)
  set (_PKG_CONFIG_COMPILER ${CMAKE_C_COMPILER})
  configure_file (
      ${HDF_RESOURCES_DIR}/libh5cc.in
      ${HDF5_BINARY_DIR}/CMakeFiles/h5hlcc
      @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5hlcc
      DESTINATION ${HDF5_INSTALL_BIN_DIR}
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT hllibraries
  )
endif ()
endif ()
