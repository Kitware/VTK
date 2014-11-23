
#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
if (NOT HDF5_INSTALL_NO_DEVELOPMENT)
  install (
      FILES ${PROJECT_BINARY_DIR}/H5pubconf.h
      DESTINATION ${HDF5_INSTALL_INCLUDE_DIR}
      COMPONENT headers
  )
endif (NOT HDF5_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  install (
      EXPORT ${HDF5_EXPORTED_TARGETS}
      DESTINATION ${HDF5_INSTALL_CMAKE_DIR}/${HDF5_PACKAGE}
      FILE ${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-targets.cmake
      COMPONENT configinstall
  )
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Export all exported targets to the build tree for use by parent project
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  EXPORT (
      TARGETS ${HDF5_LIBRARIES_TO_EXPORT} ${HDF5_LIB_DEPENDENCIES}
      FILE ${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-targets.cmake
  )
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Configure the hdf5-config.cmake file for the build directory
#-----------------------------------------------------------------------------
set (HDF5_INCLUDES_BUILD_TIME
    ${HDF5_SRC_DIR} ${HDF5_CPP_SRC_DIR} ${HDF5_HL_SRC_DIR}
    ${HDF5_TOOLS_SRC_DIR} ${HDF5_BINARY_DIR}
)
set (HDF5_VERSION_STRING @HDF5_PACKAGE_VERSION@)
set (HDF5_VERSION_MAJOR  @HDF5_PACKAGE_VERSION_MAJOR@)
set (HDF5_VERSION_MINOR  @HDF5_PACKAGE_VERSION_MINOR@)

configure_file (
    ${HDF5_RESOURCES_DIR}/hdf5-config.cmake.build.in 
    ${HDF5_BINARY_DIR}/${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-config.cmake @ONLY
)

#-----------------------------------------------------------------------------
# Configure the FindHDF5.cmake file for the install directory
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  configure_file (
      ${HDF5_RESOURCES_DIR}/FindHDF5.cmake.in 
      ${HDF5_BINARY_DIR}/CMakeFiles/FindHDF5${HDF_PACKAGE_EXT}.cmake @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/FindHDF5${HDF_PACKAGE_EXT}.cmake
      DESTINATION ${HDF5_INSTALL_CMAKE_DIR}/${HDF5_PACKAGE}
      COMPONENT configinstall
  )
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Configure the hdf5-config.cmake file for the install directory
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  configure_file (
      ${HDF5_RESOURCES_DIR}/hdf5-config.cmake.install.in
      ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-config.cmake @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-config.cmake
      DESTINATION ${HDF5_INSTALL_CMAKE_DIR}/${HDF5_PACKAGE}
      COMPONENT configinstall
  )
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Configure the hdf5-config-version .cmake file for the install directory
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  configure_file (
      ${HDF5_RESOURCES_DIR}/hdf5-config-version.cmake.in
      ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-config-version.cmake @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_PACKAGE}${HDF_PACKAGE_EXT}-config-version.cmake
      DESTINATION ${HDF5_INSTALL_CMAKE_DIR}/${HDF5_PACKAGE}
      COMPONENT configinstall
  )
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Configure the libhdf5.settings file for the lib info
#-----------------------------------------------------------------------------
if (H5_WORDS_BIGENDIAN)
  set (BYTESEX big-endian)
else (H5_WORDS_BIGENDIAN)
  set (BYTESEX little-endian)
endif (H5_WORDS_BIGENDIAN)
configure_file (
    ${HDF5_RESOURCES_DIR}/libhdf5.settings.cmake.in 
    ${HDF5_BINARY_DIR}/libhdf5.settings @ONLY
)
install (
    FILES ${HDF5_BINARY_DIR}/libhdf5.settings
    DESTINATION ${HDF5_INSTALL_CMAKE_DIR}/${HDF5_PACKAGE}
    COMPONENT libraries
)

#-----------------------------------------------------------------------------
# Configure the HDF518_Examples.cmake file and the examples
#-----------------------------------------------------------------------------
option (HDF5_PACK_EXAMPLES  "Package the HDF5 Library Examples Compressed File" OFF)
mark_as_advanced(HDF5_PACK_EXAMPLES)
if (HDF5_PACK_EXAMPLES)
  configure_file (
      ${HDF5_RESOURCES_DIR}/HDF518_Examples.cmake.in 
      ${HDF5_BINARY_DIR}/HDF518_Examples.cmake @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/HDF518_Examples.cmake
      DESTINATION ${HDF5_INSTALL_DATA_DIR}
      COMPONENT hdfdocuments
  )
  if (EXISTS "${HDF5_EXAMPLES_COMPRESSED_DIR}/${HDF5_EXAMPLES_COMPRESSED}")
    install (
        FILES
            ${HDF5_EXAMPLES_COMPRESSED_DIR}/${HDF5_EXAMPLES_COMPRESSED}
            ${HDF5_SOURCE_DIR}/release_docs/USING_CMake_Examples.txt
        DESTINATION ${HDF5_INSTALL_DATA_DIR}
        COMPONENT hdfdocuments
    )
  endif (EXISTS "${HDF5_EXAMPLES_COMPRESSED_DIR}/${HDF5_EXAMPLES_COMPRESSED}")
endif (HDF5_PACK_EXAMPLES)

#-----------------------------------------------------------------------------
# Configure the README.txt file for the binary package
#-----------------------------------------------------------------------------
set (BINARY_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set (BINARY_PLATFORM "${CMAKE_SYSTEM_NAME}")
if (WIN32)
  set (BINARY_EXAMPLE_ENDING "zip")
  set (BINARY_INSTALL_ENDING "exe")
  if (CMAKE_CL_64)
    set (BINARY_SYSTEM_NAME "win64")
  else (CMAKE_CL_64)
    set (BINARY_SYSTEM_NAME "win32")
  endif (CMAKE_CL_64)
  if (${CMAKE_SYSTEM_VERSION} MATCHES "6.1")
    set (BINARY_PLATFORM "${BINARY_PLATFORM} 7")
  elseif (${CMAKE_SYSTEM_VERSION} MATCHES "6.2")
    set (BINARY_PLATFORM "${BINARY_PLATFORM} 8")
  endif (${CMAKE_SYSTEM_VERSION} MATCHES "6.1")
  set (BINARY_PLATFORM "${BINARY_PLATFORM} ${MSVC_C_ARCHITECTURE_ID}")
  if (${CMAKE_C_COMPILER_VERSION} MATCHES "16.*")
    set (BINARY_PLATFORM "${BINARY_PLATFORM}, using VISUAL STUDIO 2010")
  elseif (${CMAKE_C_COMPILER_VERSION} MATCHES "15.*")
    set (BINARY_PLATFORM "${BINARY_PLATFORM}, using VISUAL STUDIO 2008")
  elseif (${CMAKE_C_COMPILER_VERSION} MATCHES "17.*")
    set (BINARY_PLATFORM "${BINARY_PLATFORM}, using VISUAL STUDIO 2012")
  else (${CMAKE_C_COMPILER_VERSION} MATCHES "16.*")
    set (BINARY_PLATFORM "${BINARY_PLATFORM}, using VISUAL STUDIO ${CMAKE_C_COMPILER_VERSION}")
  endif (${CMAKE_C_COMPILER_VERSION} MATCHES "16.*")
elseif (APPLE)
  set (BINARY_EXAMPLE_ENDING "tar.gz")
  set (BINARY_INSTALL_ENDING "dmg")
  set (BINARY_PLATFORM "${BINARY_PLATFORM} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}")
  set (BINARY_PLATFORM "${BINARY_PLATFORM}, using ${CMAKE_C_COMPILER_ID} C ${CMAKE_C_COMPILER_VERSION}")
else (WIN32)
  set (BINARY_EXAMPLE_ENDING "tar.gz")
  set (BINARY_INSTALL_ENDING "sh")
  set (BINARY_PLATFORM "${BINARY_PLATFORM} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}")
  set (BINARY_PLATFORM "${BINARY_PLATFORM}, using ${CMAKE_C_COMPILER_ID} C ${CMAKE_C_COMPILER_VERSION}")
endif (WIN32)
if (HDF4_BUILD_FORTRAN)
  set (BINARY_PLATFORM "${BINARY_PLATFORM} / ${CMAKE_Fortran_COMPILER_ID} Fortran")
endif (HDF4_BUILD_FORTRAN)

configure_file (
    ${HDF5_RESOURCES_DIR}/README.txt.cmake.in 
    ${HDF5_BINARY_DIR}/README.txt @ONLY
)

#-----------------------------------------------------------------------------
# Add Document File(s) to CMake Install
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  install (
      FILES
          ${HDF5_SOURCE_DIR}/COPYING
      DESTINATION ${HDF5_INSTALL_DATA_DIR}
      COMPONENT hdfdocuments
  )
  if (EXISTS "${HDF5_SOURCE_DIR}/release_docs" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/release_docs")
    set (release_files
        ${HDF5_SOURCE_DIR}/release_docs/USING_HDF5_CMake.txt
        ${HDF5_SOURCE_DIR}/release_docs/COPYING
        ${HDF5_SOURCE_DIR}/release_docs/RELEASE.txt
    )
    if (WIN32)
      set (release_files
          ${release_files}
          ${HDF5_SOURCE_DIR}/release_docs/USING_HDF5_VS.txt
      )
    endif (WIN32)
    if (HDF5_PACK_INSTALL_DOCS)
      set (release_files
          ${release_files}
          ${HDF5_SOURCE_DIR}/release_docs/INSTALL_CMake.txt
          ${HDF5_SOURCE_DIR}/release_docs/HISTORY-1_8.txt
          ${HDF5_SOURCE_DIR}/release_docs/INSTALL
      )
      if (WIN32)
        set (release_files
            ${release_files}
            ${HDF5_SOURCE_DIR}/release_docs/INSTALL_Windows.txt
        )
      endif (WIN32)
      if (CYGWIN)
        set (release_files
            ${release_files}
            ${HDF5_SOURCE_DIR}/release_docs/INSTALL_Cygwin.txt
        )
      endif (CYGWIN)
      if (HDF5_ENABLE_PARALLEL)
        set (release_files
            ${release_files}
            ${HDF5_SOURCE_DIR}/release_docs/INSTALL_parallel
        )
      endif (HDF5_ENABLE_PARALLEL)
    endif (HDF5_PACK_INSTALL_DOCS)
    install (
        FILES ${release_files}
        DESTINATION ${HDF5_INSTALL_DATA_DIR}
        COMPONENT hdfdocuments
    )
  endif (EXISTS "${HDF5_SOURCE_DIR}/release_docs" AND IS_DIRECTORY "${HDF5_SOURCE_DIR}/release_docs")
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

#-----------------------------------------------------------------------------
# Set the cpack variables
#-----------------------------------------------------------------------------
if (NOT HDF5_EXTERNALLY_CONFIGURED AND NOT HDF5_NO_PACKAGES)
  set (CPACK_PACKAGE_VENDOR "HDF_Group")
  set (CPACK_PACKAGE_NAME "${HDF5_PACKAGE_NAME}")
  if (CDASH_LOCAL)
    set (CPACK_PACKAGE_VERSION "${HDF5_PACKAGE_VERSION}")
  else (CDASH_LOCAL)
    set (CPACK_PACKAGE_VERSION "${HDF5_PACKAGE_VERSION_STRING}")
  endif (CDASH_LOCAL)
  set (CPACK_PACKAGE_VERSION_MAJOR "${HDF5_PACKAGE_VERSION_MAJOR}")
  set (CPACK_PACKAGE_VERSION_MINOR "${HDF5_PACKAGE_VERSION_MINOR}")
  set (CPACK_PACKAGE_VERSION_PATCH "")
  if (EXISTS "${HDF5_SOURCE_DIR}/release_docs")
    set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/release_docs/RELEASE.txt")
    set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/release_docs/COPYING")
    set (CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/release_docs/RELEASE.txt")
  endif (EXISTS "${HDF5_SOURCE_DIR}/release_docs")
  set (CPACK_PACKAGE_RELOCATABLE TRUE)
  set (CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_VENDOR}/${CPACK_PACKAGE_NAME}/${CPACK_PACKAGE_VERSION}")
  set (CPACK_PACKAGE_ICON "${HDF5_RESOURCES_DIR}/hdf.bmp")

  set (CPACK_GENERATOR "TGZ") 
  if (WIN32)
    LIST (APPEND CPACK_GENERATOR "NSIS") 
    # Installers for 32- vs. 64-bit CMake:
    #  - Root install directory (displayed to end user at installer-run time)
    #  - "NSIS package/display name" (text used in the installer GUI)
    #  - Registry key used to store info about the installation
    set (CPACK_NSIS_PACKAGE_NAME "${HDF5_PACKAGE_STRING}")
    if (CMAKE_CL_64)
      set (CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
      set (CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION} (Win64)")
    else (CMAKE_CL_64)
      set (CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
      set (CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
    endif (CMAKE_CL_64)
    # set the install/unistall icon used for the installer itself
    # There is a bug in NSI that does not handle full unix paths properly.
    set (CPACK_NSIS_MUI_ICON "${HDF5_RESOURCES_DIR}\\\\hdf.ico")
    set (CPACK_NSIS_MUI_UNIICON "${HDF5_RESOURCES_DIR}\\\\hdf.ico")
    # set the package header icon for MUI
    set (CPACK_PACKAGE_ICON "${HDF5_RESOURCES_DIR}\\\\hdf.bmp")
    set (CPACK_NSIS_DISPLAY_NAME "@CPACK_NSIS_PACKAGE_NAME@")
    set (CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_VENDOR}\\\\${CPACK_PACKAGE_NAME}\\\\${CPACK_PACKAGE_VERSION}")
    set (CPACK_MONOLITHIC_INSTALL ON)
    set (CPACK_NSIS_CONTACT "${HDF5_PACKAGE_BUGREPORT}")
    set (CPACK_NSIS_MODIFY_PATH ON)
  elseif (APPLE)
    LIST (APPEND CPACK_GENERATOR "DragNDrop") 
    set (CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
    set (CPACK_PACKAGING_INSTALL_PREFIX "/${CPACK_PACKAGE_INSTALL_DIRECTORY}")
    set (CPACK_PACKAGE_ICON "${HDF5_RESOURCES_DIR}/hdf.icns")
    
    if (HDF5_PACK_MACOSX_BUNDLE)
      LIST (APPEND CPACK_GENERATOR "Bundle")
      set (CPACK_BUNDLE_NAME "${HDF5_PACKAGE_STRING}")
      set (CPACK_BUNDLE_LOCATION "/")    # make sure CMAKE_INSTALL_PREFIX ends in /
      set (CMAKE_INSTALL_PREFIX "/${CPACK_BUNDLE_NAME}.framework/Versions/${CPACK_PACKAGE_VERSION}/${CPACK_PACKAGE_NAME}/")
      set (CPACK_BUNDLE_ICON "${HDF5_RESOURCES_DIR}/hdf.icns")
      set (CPACK_BUNDLE_PLIST "${HDF5_BINARY_DIR}/CMakeFiles/Info.plist")
      set (CPACK_APPLE_GUI_INFO_STRING "HDF5 (Hierarchical Data Format 5) Software Library and Utilities")
      set (CPACK_APPLE_GUI_COPYRIGHT "Copyright © 2006-2014 by The HDF Group. All rights reserved.")
      set (CPACK_SHORT_VERSION_STRING "${CPACK_PACKAGE_VERSION}")
      set (CPACK_APPLE_GUI_BUNDLE_NAME "${HDF5_PACKAGE_STRING}")
      set (CPACK_APPLE_GUI_VERSION_STRING "${CPACK_PACKAGE_VERSION_STRING}")
      set (CPACK_APPLE_GUI_SHORT_VERSION_STRING "${CPACK_PACKAGE_VERSION}")
      #-----------------------------------------------------------------------------
      # Configure the Info.plist file for the install bundle
      #-----------------------------------------------------------------------------
      configure_file (
          ${HDF5_RESOURCES_DIR}/CPack.Info.plist.in
          ${HDF5_BINARY_DIR}/CMakeFiles/Info.plist @ONLY
      )
      configure_file (
          ${HDF5_RESOURCES_DIR}/PkgInfo.in
          ${HDF5_BINARY_DIR}/CMakeFiles/PkgInfo @ONLY
      )
      configure_file (
          ${HDF5_RESOURCES_DIR}/version.plist.in
          ${HDF5_BINARY_DIR}/CMakeFiles/version.plist @ONLY
      )
      install (
          FILES ${HDF5_BINARY_DIR}/CMakeFiles/PkgInfo
                ${HDF5_BINARY_DIR}/CMakeFiles/version.plist
          DESTINATION ..
      )
    ENDIF(HDF5_PACK_MACOSX_BUNDLE)
  else (WIN32)
    LIST (APPEND CPACK_GENERATOR "STGZ") 
    set (CPACK_PACKAGING_INSTALL_PREFIX "/${CPACK_PACKAGE_INSTALL_DIRECTORY}")
    set (CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)

    set (CPACK_DEBIAN_PACKAGE_SECTION "Libraries")
    set (CPACK_DEBIAN_PACKAGE_MAINTAINER "${HDF5_PACKAGE_BUGREPORT}")

#    LIST (APPEND CPACK_GENERATOR "RPM") 
    set (CPACK_RPM_PACKAGE_RELEASE "1")
    set (CPACK_RPM_COMPONENT_INSTALL ON)
    set (CPACK_RPM_PACKAGE_RELOCATABLE ON)
    set (CPACK_RPM_PACKAGE_LICENSE "BSD-style")
    set (CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    set (CPACK_RPM_PACKAGE_URL "${HDF5_PACKAGE_URL}")
    set (CPACK_RPM_PACKAGE_SUMMARY "HDF5 is a unique technology suite that makes possible the management of extremely large and complex data collections.")
    set (CPACK_RPM_PACKAGE_DESCRIPTION 
        "The HDF5 technology suite includes:

    * A versatile data model that can represent very complex data objects and a wide variety of metadata.

    * A completely portable file format with no limit on the number or size of data objects in the collection.

    * A software library that runs on a range of computational platforms, from laptops to massively parallel systems, and implements a high-level API with C, C++, Fortran 90, and Java interfaces.

    * A rich set of integrated performance features that allow for access time and storage space optimizations.

    * Tools and applications for managing, manipulating, viewing, and analyzing the data in the collection.

The HDF5 data model, file format, API, library, and tools are open and distributed without charge.
"
    )
    
    #-----------------------------------------------------------------------------
    # Configure the spec file for the install RPM
    #-----------------------------------------------------------------------------
#    configure_file ("${HDF5_RESOURCES_DIR}/hdf5.spec.in" "${CMAKE_CURRENT_BINARY_DIR}/${HDF5_PACKAGE_NAME}.spec" @ONLY IMMEDIATE)
#    set (CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/${HDF5_PACKAGE_NAME}.spec")
  endif (WIN32)
  
  # By default, do not warn when built on machines using only VS Express:
  if (NOT DEFINED CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)
    set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
  ENDIF(NOT DEFINED CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)
  INCLUDE(InstallRequiredSystemLibraries)

  set (CPACK_INSTALL_CMAKE_PROJECTS "${HDF5_BINARY_DIR};HDF5;ALL;/")
  
  if (HDF5_PACKAGE_EXTLIBS)
    if (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "SVN" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
      if (ZLIB_FOUND AND ZLIB_USE_EXTERNAL)
        set (CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${ZLIB_INCLUDE_DIR_GEN};ZLIB;libraries;/")
        set (CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${ZLIB_INCLUDE_DIR_GEN};ZLIB;headers;/")
        set (CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${ZLIB_INCLUDE_DIR_GEN};ZLIB;configinstall;/")
      endif (ZLIB_FOUND AND ZLIB_USE_EXTERNAL)
      if (SZIP_FOUND AND SZIP_USE_EXTERNAL)
        set (CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${SZIP_INCLUDE_DIR_GEN};SZIP;libraries;/")
        set (CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${SZIP_INCLUDE_DIR_GEN};SZIP;headers;/")
        set (CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${SZIP_INCLUDE_DIR_GEN};SZIP;configinstall;/")
      endif (SZIP_FOUND AND SZIP_USE_EXTERNAL)
    endif (HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "SVN" OR HDF5_ALLOW_EXTERNAL_SUPPORT MATCHES "TGZ")
  endif (HDF5_PACKAGE_EXTLIBS)
  
  include (CPack)

  #---------------------------------------------------------------------------
  # Now list the cpack commands
  #---------------------------------------------------------------------------
  CPACK_ADD_COMPONENT (hdfapplications 
      DISPLAY_NAME "HDF5 Applications" 
      DEPENDS libraries
      GROUP Applications
  )
  CPACK_ADD_COMPONENT (libraries 
      DISPLAY_NAME "HDF5 Libraries"
      GROUP Runtime
  )
  CPACK_ADD_COMPONENT (headers 
      DISPLAY_NAME "HDF5 Headers" 
      DEPENDS libraries
      GROUP Development
  )
  CPACK_ADD_COMPONENT (hdfdocuments 
      DISPLAY_NAME "HDF5 Documents"
      GROUP Documents
  )
  CPACK_ADD_COMPONENT (configinstall 
      DISPLAY_NAME "HDF5 CMake files" 
      DEPENDS libraries
      GROUP Development
  )
  
  if (HDF5_BUILD_FORTRAN)
    CPACK_ADD_COMPONENT (fortlibraries 
        DISPLAY_NAME "HDF5 Fortran Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (fortheaders 
        DISPLAY_NAME "HDF5 Fortran Headers" 
        DEPENDS fortlibraries
        GROUP Development
    )
  endif (HDF5_BUILD_FORTRAN)
  
  if (HDF5_BUILD_CPP_LIB)
    CPACK_ADD_COMPONENT (cpplibraries 
        DISPLAY_NAME "HDF5 C++ Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (cppheaders 
        DISPLAY_NAME "HDF5 C++ Headers" 
        DEPENDS cpplibraries
        GROUP Development
    )
  endif (HDF5_BUILD_CPP_LIB)
  
  if (HDF5_BUILD_TOOLS)
    CPACK_ADD_COMPONENT (toolsapplications 
        DISPLAY_NAME "HDF5 Tools Applications" 
        DEPENDS toolslibraries
        GROUP Applications
    )
    CPACK_ADD_COMPONENT (toolslibraries 
        DISPLAY_NAME "HDF5 Tools Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (toolsheaders 
        DISPLAY_NAME "HDF5 Tools Headers" 
        DEPENDS toolslibraries
        GROUP Development
    )
  endif (HDF5_BUILD_TOOLS)
  
  if (HDF5_BUILD_HL_LIB)
    CPACK_ADD_COMPONENT (hllibraries 
        DISPLAY_NAME "HDF5 HL Libraries" 
        DEPENDS libraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (hlheaders 
        DISPLAY_NAME "HDF5 HL Headers" 
        DEPENDS hllibraries
        GROUP Development
    )
    CPACK_ADD_COMPONENT (hltoolsapplications 
        DISPLAY_NAME "HDF5 HL Tools Applications" 
        DEPENDS hllibraries
        GROUP Applications
    )
    CPACK_ADD_COMPONENT (hlcpplibraries 
        DISPLAY_NAME "HDF5 HL C++ Libraries" 
        DEPENDS hllibraries
        GROUP Runtime
    )
    CPACK_ADD_COMPONENT (hlcppheaders 
        DISPLAY_NAME "HDF5 HL C++ Headers" 
        DEPENDS hlcpplibraries
        GROUP Development
    )
    CPACK_ADD_COMPONENT (hlfortlibraries 
        DISPLAY_NAME "HDF5 HL Fortran Libraries" 
        DEPENDS fortlibraries
        GROUP Runtime
    )
  endif (HDF5_BUILD_HL_LIB)
  
endif (NOT HDF5_EXTERNALLY_CONFIGURED AND NOT HDF5_NO_PACKAGES)
