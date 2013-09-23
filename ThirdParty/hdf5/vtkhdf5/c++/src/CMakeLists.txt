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
CONFIGURE_FILE (${HDF5_SOURCE_DIR}/Resources/H5cxx_config.h.in
                ${HDF5_BINARY_DIR}/H5cxx_pubconf.h
)

#-----------------------------------------------------------------------------
# Define cpp Library
#-----------------------------------------------------------------------------
SET (CPP_SRCS
    ${HDF5_CPP_SOURCE_DIR}/src/H5AbstractDs.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5ArrayType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5AtomType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5Attribute.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5CommonFG.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5CompType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5DataSet.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5DataSpace.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5DataType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5DcreatProp.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5DxferProp.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5EnumType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5Exception.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5FaccProp.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5FcreatProp.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5File.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5FloatType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5Group.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5IdComponent.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5IntType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5Library.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5Object.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5PredType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5PropList.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5StrType.cpp
    ${HDF5_CPP_SOURCE_DIR}/src/H5VarLenType.cpp
)

SET (CPP_HDRS
    ${HDF5_CPP_SOURCE_DIR}/src/H5AbstractDs.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Alltypes.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5ArrayType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5AtomType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Attribute.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Classes.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5CommonFG.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5CompType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Cpp.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5CppDoc.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5DataSet.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5DataSpace.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5DataType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5DcreatProp.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5DxferProp.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5EnumType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Exception.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5FaccProp.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5FcreatProp.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5File.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5FloatType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Group.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5IdComponent.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Include.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5IntType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Library.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5Object.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5PredType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5PropList.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5StrType.h
    ${HDF5_CPP_SOURCE_DIR}/src/H5VarLenType.h
)

ADD_LIBRARY (${HDF5_CPP_LIB_TARGET} ${LIB_TYPE} ${CPP_SRCS} ${CPP_HDRS})
TARGET_LINK_LIBRARIES (${HDF5_CPP_LIB_TARGET} ${HDF5_LIB_TARGET})
SET_GLOBAL_VARIABLE (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_CPP_LIB_TARGET}")
H5_SET_LIB_OPTIONS (${HDF5_CPP_LIB_TARGET} ${HDF5_CPP_LIB_NAME} ${LIB_TYPE})

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install 
#-----------------------------------------------------------------------------
IF (NOT VTK_INSTALL_NO_DEVELOPMENT)
INSTALL (
    FILES
        ${CPP_HDRS}
    DESTINATION
        ${VTK_INSTALL_INCLUDE_DIR}/vtkhdf5/cpp
)
ENDIF (NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
IF (HDF5_EXPORTED_TARGETS AND NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL (
      TARGETS
          ${HDF5_CPP_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
  )
ENDIF (HDF5_EXPORTED_TARGETS AND NOT VTK_INSTALL_NO_LIBRARIES)

