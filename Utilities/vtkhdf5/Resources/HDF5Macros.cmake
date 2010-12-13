#-------------------------------------------------------------------------------
MACRO (SET_GLOBAL_VARIABLE name value)
  SET (${name} ${value} CACHE INTERNAL "Used to pass variables between directories" FORCE)
ENDMACRO (SET_GLOBAL_VARIABLE)

#-------------------------------------------------------------------------------
MACRO (IDE_GENERATED_PROPERTIES SOURCE_PATH HEADERS SOURCES)
  #set(source_group_path "Source/AIM/${NAME}")
  STRING (REPLACE "/" "\\\\" source_group_path ${SOURCE_PATH})
  source_group(${source_group_path} FILES ${HEADERS} ${SOURCES})

  #-- The following is needed if we ever start to use OS X Frameworks but only
  #--  works on CMake 2.6 and greater
  #SET_PROPERTY (SOURCE ${HEADERS}
  #       PROPERTY MACOSX_PACKAGE_LOCATION Headers/${NAME}
  #)
ENDMACRO (IDE_GENERATED_PROPERTIES)

#-------------------------------------------------------------------------------
MACRO (IDE_SOURCE_PROPERTIES SOURCE_PATH HEADERS SOURCES)
  #  INSTALL (FILES ${HEADERS}
  #       DESTINATION include/R3D/${NAME}
  #       COMPONENT Headers       
  #  )

  STRING (REPLACE "/" "\\\\" source_group_path ${SOURCE_PATH}  )
  source_group (${source_group_path} FILES ${HEADERS} ${SOURCES})

  #-- The following is needed if we ever start to use OS X Frameworks but only
  #--  works on CMake 2.6 and greater
  #SET_PROPERTY (SOURCE ${HEADERS}
  #       PROPERTY MACOSX_PACKAGE_LOCATION Headers/${NAME}
  #)
ENDMACRO (IDE_SOURCE_PROPERTIES)

#-------------------------------------------------------------------------------
MACRO (H5_NAMING target)
  IF (WIN32 AND NOT MINGW)
    IF (BUILD_SHARED_LIBS)
      IF (H5_LEGACY_NAMING)
        SET_TARGET_PROPERTIES (${target} PROPERTIES OUTPUT_NAME "dll")
        SET_TARGET_PROPERTIES (${target} PROPERTIES PREFIX "${target}")
      ELSE (H5_LEGACY_NAMING)
        SET_TARGET_PROPERTIES (${target} PROPERTIES OUTPUT_NAME "${target}dll")
      ENDIF (H5_LEGACY_NAMING)
    ENDIF (BUILD_SHARED_LIBS)
  ENDIF (WIN32 AND NOT MINGW)
ENDMACRO (H5_NAMING)

#-------------------------------------------------------------------------------
MACRO (H5_SET_LIB_OPTIONS libtarget libname libtype)
  # message (STATUS "${libname} libtype: ${libtype}")
  IF (${libtype} MATCHES "SHARED")
    IF (WIN32 AND NOT MINGW)
      IF (H5_LEGACY_NAMING)
        SET (LIB_RELEASE_NAME "${libname}dll")
        SET (LIB_DEBUG_NAME "${libname}ddll")
      ELSE (H5_LEGACY_NAMING)
        SET (LIB_RELEASE_NAME "${libname}")
        SET (LIB_DEBUG_NAME "${libname}_D")
      ENDIF (H5_LEGACY_NAMING)
    ELSE (WIN32 AND NOT MINGW)
      SET (LIB_RELEASE_NAME "${libname}")
      SET (LIB_DEBUG_NAME "${libname}_debug")
    ENDIF (WIN32 AND NOT MINGW)
  ELSE (${libtype} MATCHES "SHARED")
    IF (WIN32 AND NOT MINGW)
      IF (H5_LEGACY_NAMING)
        SET (LIB_RELEASE_NAME "${libname}")
        SET (LIB_DEBUG_NAME "${libname}d")
      ELSE (H5_LEGACY_NAMING)
        SET (LIB_RELEASE_NAME "lib${libname}")
        SET (LIB_DEBUG_NAME "lib${libname}_D")
      ENDIF (H5_LEGACY_NAMING)
    ELSE (WIN32 AND NOT MINGW)
      SET (LIB_RELEASE_NAME "lib${libname}")
      SET (LIB_DEBUG_NAME "lib${libname}_debug")
    ENDIF (WIN32 AND NOT MINGW)
  ENDIF (${libtype} MATCHES "SHARED")
  
  # vtkhdf5 has vtk prefix but no other encoding
  
  #----- Use MSVC Naming conventions for Shared Libraries
  IF (MINGW AND BUILD_SHARED_LIBS)
    SET_TARGET_PROPERTIES (${libtarget}
        PROPERTIES
        IMPORT_SUFFIX ".lib"
        IMPORT_PREFIX ""
        PREFIX ""
    )
  ENDIF (MINGW AND BUILD_SHARED_LIBS)

  IF (BUILD_SHARED_LIBS)
    IF (WIN32)
      SET (LIBHDF_VERSION HDF5_PACKAGE_VERSION_MAJOR)
    ELSE (WIN32)
      SET (LIBHDF_VERSION ${HDF5_PACKAGE_VERSION})
    ENDIF (WIN32)
    SET_TARGET_PROPERTIES (${libtarget} PROPERTIES VERSION ${LIBHDF_VERSION})
    SET_TARGET_PROPERTIES (${libtarget} PROPERTIES SOVERSION ${LIBHDF_VERSION})
  ENDIF (BUILD_SHARED_LIBS)

  #-- Apple Specific install_name for libraries
  IF (APPLE)
    OPTION (HDF5_BUILD_WITH_INSTALL_NAME "Build with library install_name set to the installation path" OFF)
    IF (HDF5_BUILD_WITH_INSTALL_NAME)
      SET_TARGET_PROPERTIES(${libtarget} PROPERTIES
          LINK_FLAGS "-current_version ${HDF5_PACKAGE_VERSION} -compatibility_version ${HDF5_PACKAGE_VERSION}"
          INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
          BUILD_WITH_INSTALL_RPATH ${HDF5_BUILD_WITH_INSTALL_NAME}
      )
    ENDIF (HDF5_BUILD_WITH_INSTALL_NAME)
  ENDIF (APPLE)

ENDMACRO (H5_SET_LIB_OPTIONS)

