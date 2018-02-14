# XXX(kitware): cmake versions are handled by VTK.
if (FALSE)
cmake_minimum_required (VERSION 3.2.2)
endif ()
PROJECT (HDF5_HL_SRC)

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
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TBpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LDpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/hdf5_hl.h
    ${HDF5_HL_SRC_SOURCE_DIR}/vtk_hdf5_hl_mangle.h
)

# XXX(kitware): Use VTK's module system.
if (FALSE)
add_library (${HDF5_HL_LIB_TARGET} STATIC ${HL_SOURCES} ${HL_HEADERS})
TARGET_C_PROPERTIES (${HDF5_HL_LIB_TARGET} STATIC " " " ")
target_link_libraries (${HDF5_HL_LIB_TARGET} ${HDF5_LIB_TARGET})
H5_SET_LIB_OPTIONS (${HDF5_HL_LIB_TARGET} ${HDF5_HL_LIB_NAME} STATIC)
set_target_properties (${HDF5_HL_LIB_TARGET} PROPERTIES
    FOLDER libraries/hl
    INTERFACE_INCLUDE_DIRECTORIES "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)
set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIB_TARGET}")
set (install_targets ${HDF5_HL_LIB_TARGET})
else ()
vtk_add_library(vtkhdf5_hl_src ${HL_SOURCES} ${HL_HEADERS})
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${HL_HEADERS}
    DESTINATION  "${VTK_INSTALL_INCLUDE_DIR}/vtkhdf5/hl/src"
    COMPONENT Development)
endif()
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

if (DEFINED VTK_CUSTOM_LIBRARY_SUFFIX)
  set(_lib_suffix "${VTK_CUSTOM_LIBRARY_SUFFIX}")
else ()
  set(_lib_suffix "-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
endif ()
set_property(TARGET vtkhdf5_hl_src PROPERTY OUTPUT_NAME vtkhdf5_hl${_lib_suffix})
endif ()

# XXX(kitware): VTK handles shared/static flags.
if (BUILD_SHARED_LIBS AND FALSE)
  add_library (${HDF5_HL_LIBSH_TARGET} SHARED ${HL_SOURCES} ${HL_HEADERS})
  TARGET_C_PROPERTIES (${HDF5_HL_LIBSH_TARGET} SHARED " " " ")
  target_link_libraries (${HDF5_HL_LIBSH_TARGET} ${HDF5_LIBSH_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_HL_LIBSH_TARGET} ${HDF5_HL_LIB_NAME} SHARED ${HDF5_HL_PACKAGE_SOVERSION})
  set_target_properties (${HDF5_HL_LIBSH_TARGET} PROPERTIES
      FOLDER libraries/hl
      COMPILE_DEFINITIONS "H5_BUILT_AS_DYNAMIC_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
      INTERFACE_COMPILE_DEFINITIONS H5_BUILT_AS_DYNAMIC_LIB=1
  )
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIBSH_TARGET}")
  set (install_targets ${install_targets} ${HDF5_HL_LIBSH_TARGET})
endif ()

# XXX(kitware): VTK handles installation.
if (FALSE)
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
  INSTALL_TARGET_PDB (${HDF5_HL_LIB_TARGET} ${HDF5_INSTALL_BIN_DIR} hllibraries)

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
endif ()
