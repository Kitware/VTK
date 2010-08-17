cmake_minimum_required (VERSION 2.8)
PROJECT (HDF5_CPP_SRC)

#-----------------------------------------------------------------------------
# Shared/Static Libs
#-----------------------------------------------------------------------------
IF (BUILD_SHARED_LIBS)
  SET (CPP_BUILT_AS_DYNAMIC_LIB 1)
ENDIF (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Generate configure file
#-----------------------------------------------------------------------------
CONFIGURE_FILE (${HDF5_RESOURCES_DIR}/H5cxx_config.h.in
                ${HDF5_BINARY_DIR}/H5cxx_pubconf.h
)

#-----------------------------------------------------------------------------
# Define cpp Library
#-----------------------------------------------------------------------------
SET (CPP_SRCS
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5AbstractDs.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5ArrayType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5AtomType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Attribute.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5CommonFG.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5CompType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DataSet.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DataSpace.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DataType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DcreatProp.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DxferProp.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5EnumType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Exception.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5FaccProp.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5FcreatProp.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5File.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5FloatType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Group.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5IdComponent.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5IntType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Library.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Object.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PredType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PropList.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5StrType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5VarLenType.cpp
)

SET (CPP_HDRS
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5AbstractDs.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Alltypes.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5ArrayType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5AtomType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Attribute.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Classes.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5CommonFG.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5CompType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Cpp.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5CppDoc.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DataSet.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DataSpace.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DataType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DcreatProp.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5DxferProp.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5EnumType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Exception.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5FaccProp.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5FcreatProp.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5File.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5FloatType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Group.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5IdComponent.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Include.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5IntType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Library.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Object.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PredType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PropList.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5StrType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5VarLenType.h
)

ADD_LIBRARY (${HDF5_CPP_LIB_TARGET} ${LIB_TYPE} ${CPP_SRCS} ${CPP_HDRS})
TARGET_LINK_LIBRARIES (${HDF5_CPP_LIB_TARGET} ${HDF5_LIB_TARGET})
SET_GLOBAL_VARIABLE (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_CPP_LIB_TARGET}")
H5_SET_LIB_OPTIONS (${HDF5_CPP_LIB_TARGET} ${HDF5_CPP_LIB_NAME} ${LIB_TYPE})

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install 
#-----------------------------------------------------------------------------
INSTALL (
    FILES
        ${CPP_HDRS}
    DESTINATION
        ${HDF5_INSTALL_INCLUDE_DIR}/cpp
    COMPONENT
        cppheaders
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
IF (HDF5_EXPORTED_TARGETS)
  INSTALL (
      TARGETS
          ${HDF5_CPP_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT cpplibraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT cpplibraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT cpplibraries
  )
ENDIF (HDF5_EXPORTED_TARGETS)

