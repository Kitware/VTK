cmake_minimum_required (VERSION 2.8)
PROJECT (HDF5_HL_CPP_SRC)

#-----------------------------------------------------------------------------
# Define Sources
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_HL_SRC_DIR}/src)
INCLUDE_DIRECTORIES (${HDF5_HL_CPP_SRC_SOURCE_DIR})

SET (HDF5_HL_CPP_SRCS ${HDF5_HL_CPP_SRC_SOURCE_DIR}/H5PacketTable.cpp)
SET (HDF5_HL_CPP_HDRS ${HDF5_HL_CPP_SRC_SOURCE_DIR}/H5PacketTable.h)

ADD_LIBRARY ( ${HDF5_HL_CPP_LIB_TARGET} ${LIB_TYPE} ${HDF5_HL_CPP_SRCS})
TARGET_LINK_LIBRARIES (
    ${HDF5_HL_CPP_LIB_TARGET}
    ${HDF5_HL_LIB_TARGET}
    ${HDF5_LIB_TARGET}
)
SET_GLOBAL_VARIABLE (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_CPP_LIB_TARGET}")
H5_SET_LIB_OPTIONS (${HDF5_HL_CPP_LIB_TARGET} ${HDF5_HL_CPP_LIB_NAME} ${LIB_TYPE})

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install 
#-----------------------------------------------------------------------------
INSTALL (
    FILES
        ${HDF5_HL_CPP_HDRS}
    DESTINATION
        ${HDF5_INSTALL_INCLUDE_DIR}/hl/cpp
    COMPONENT
        hlcppheaders
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
IF (HDF5_EXPORTED_TARGETS)
  INSTALL (
      TARGETS
          ${HDF5_HL_CPP_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hlcpplibraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hlcpplibraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT hlcpplibraries
  )
ENDIF (HDF5_EXPORTED_TARGETS)
