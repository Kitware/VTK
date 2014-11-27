PROJECT (HDF5_CPP_SRC)

#-----------------------------------------------------------------------------
# Shared/Static Libs
#-----------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
  set (CPP_BUILT_AS_DYNAMIC_LIB 1)
endif (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Generate configure file
#-----------------------------------------------------------------------------
configure_file (${HDF5_RESOURCES_DIR}/H5cxx_config.h.in
                ${HDF5_BINARY_DIR}/H5cxx_pubconf.h
)

#-----------------------------------------------------------------------------
# Define cpp Library
#-----------------------------------------------------------------------------
set (CPP_SRCS
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
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Location.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Object.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PredType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PropList.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5StrType.cpp
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5VarLenType.cpp
)

set (CPP_HDRS
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
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Location.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5Object.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PredType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5PropList.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5StrType.h
    ${HDF5_CPP_SRC_SOURCE_DIR}/H5VarLenType.h
)

add_library (${HDF5_CPP_LIB_TARGET} ${LIB_TYPE} ${CPP_SRCS} ${CPP_HDRS})
TARGET_C_PROPERTIES (${HDF5_CPP_LIB_TARGET} " " " ")
target_link_libraries (${HDF5_CPP_LIB_TARGET} ${HDF5_LIB_TARGET})
set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_CPP_LIB_TARGET}")
H5_SET_LIB_OPTIONS (${HDF5_CPP_LIB_TARGET} ${HDF5_CPP_LIB_NAME} ${LIB_TYPE})
set_target_properties (${HDF5_CPP_LIB_TARGET} PROPERTIES
    FOLDER libraries/cpp
    INTERFACE_INCLUDE_DIRECTORIES "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

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
ENDIF ()

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS AND NOT VTK_INSTALL_NO_LIBRARIES)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_CPP_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} cpplibraries)
  endif (BUILD_SHARED_LIBS)

  install (
      TARGETS
          ${HDF5_CPP_LIB_TARGET}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
  )
endif (HDF5_EXPORTED_TARGETS)
