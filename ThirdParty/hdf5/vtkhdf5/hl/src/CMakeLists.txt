PROJECT (HDF5_HL_SRC)

#-----------------------------------------------------------------------------
# Shared Libs
#-----------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
  set (HL_BUILT_AS_DYNAMIC_LIB 1)
endif (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------
set (HL_SRCS
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DO.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DS.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IM.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LT.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTanalyze.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PT.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TB.c
)

set (HL_HEADERS
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DOpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DSpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IMpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TBpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/hdf5_hl.h
    ${HDF5_HL_SRC_SOURCE_DIR}/vtk_libhdf5_hl_mangle.h
)

add_library (${HDF5_HL_LIB_TARGET} ${LIB_TYPE} ${HL_SRCS} ${HL_HEADERS})
TARGET_C_PROPERTIES (${HDF5_HL_LIB_TARGET} " " " ")
target_link_libraries (${HDF5_HL_LIB_TARGET} ${HDF5_LIB_TARGET})
H5_SET_LIB_OPTIONS (${HDF5_HL_LIB_TARGET} ${HDF5_HL_LIB_NAME} ${LIB_TYPE})
set_target_properties (${HDF5_HL_LIB_TARGET} PROPERTIES
    FOLDER libraries/hl
    INTERFACE_INCLUDE_DIRECTORIES "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
    DEFINE_SYMBOL ${HDF5_HL_LIB_CORENAME}_EXPORTS)
set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIB_TARGET}")

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install 
#-----------------------------------------------------------------------------
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL (
      FILES
          ${HL_HEADERS}
      DESTINATION
          ${VTK_INSTALL_INCLUDE_DIR}/vtkhdf5
  )
endif()

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS AND NOT VTK_INSTALL_NO_LIBRARIES)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} hllibraries)
  endif (BUILD_SHARED_LIBS)
  
  install (
      TARGETS
          ${HDF5_HL_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT Development
  )
endif ()
