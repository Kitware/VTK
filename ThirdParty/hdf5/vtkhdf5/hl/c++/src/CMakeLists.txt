cmake_minimum_required (VERSION 2.8)
PROJECT (HDF5_HL_CPP_SRC)

#-----------------------------------------------------------------------------
# Define Sources
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_HL_SRC_DIR}/src)
INCLUDE_DIRECTORIES (${HDF5_HL_CPP_SRC_SOURCE_DIR})

set (HDF5_HL_CPP_SRCS ${HDF5_HL_CPP_SRC_SOURCE_DIR}/H5PacketTable.cpp)
set (HDF5_HL_CPP_HDRS ${HDF5_HL_CPP_SRC_SOURCE_DIR}/H5PacketTable.h)

add_library (${HDF5_HL_CPP_LIB_TARGET} ${LIB_TYPE} ${HDF5_HL_CPP_SRCS})
TARGET_C_PROPERTIES (${HDF5_HL_CPP_LIB_TARGET} " " " ")
target_link_libraries (
    ${HDF5_HL_CPP_LIB_TARGET}
    ${HDF5_HL_LIB_TARGET}
    ${HDF5_LIB_TARGET}
)
set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_CPP_LIB_TARGET}")
H5_SET_LIB_OPTIONS (${HDF5_HL_CPP_LIB_TARGET} ${HDF5_HL_CPP_LIB_NAME} ${LIB_TYPE})
set_target_properties (${HDF5_HL_CPP_LIB_TARGET} PROPERTIES
    FOLDER libraries/hl
    INTERFACE_INCLUDE_DIRECTORIES "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install 
#-----------------------------------------------------------------------------
install (
    FILES
        ${HDF5_HL_CPP_HDRS}
    DESTINATION
        ${HDF5_INSTALL_INCLUDE_DIR}
    COMPONENT
        hlcppheaders
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS AND NOT VTK_INSTALL_NO_LIBRARIES)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_CPP_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} hlcpplibraries)
  endif (BUILD_SHARED_LIBS)
  
  install (
      TARGETS
          ${HDF5_HL_CPP_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
          LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT hlcpplibraries
          ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT hlcpplibraries
          RUNTIME DESTINATION ${{VTK_INSTALL_RUNTIME_DIR} COMPONENT hlcpplibraries
  )
endif (HDF5_EXPORTED_TARGETS)
