# first include proj library
# always need
include(lib_proj.cmake)

# configure executable build
if (FALSE) # XXX(kitware): We don't build binaries
option(BUILD_CCT
  "Build cct (coordinate conversion and transformation tool)" ON)
option(BUILD_CS2CS
  "Build cs2cs (coordinate systems to coordinate systems translation tool)" ON)
option(BUILD_GEOD
  "Build geod (computation of geodesic lines)" ON)
option(BUILD_GIE
  "Build gie (geospatial integrity investigation environment)" ON)
option(BUILD_PROJ
  "Build proj (cartographic projection tool)" ON)
option(BUILD_PROJINFO
  "Build projinfo (SRS and coordinate operation metadata/query tool)" ON)
option(BUILD_PROJSYNC
  "Build projsync (synchronize transformation support data)" ON)
else ()
set(BUILD_CCT OFF)
set(BUILD_CS2CS OFF)
set(BUILD_PROJ OFF)
set(BUILD_PROJINFO OFF)
set(BUILD_PROJSYNC OFF)
set(BUILD_GEOD OFF)
set(BUILD_GIE OFF)
endif ()

if(NOT MSVC)

  if(NOT APPLE)
    # Use relative path so that package is relocatable
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${LIBDIR}")
  else()
    set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${LIBDIR}")
    # TO DO: cmake 2.8.12 introduces a way to make the install tree
    # relocatable with OSX via
    # (1) set(CMAKE_MACOSX_RPATH ON) and
    # (2) setting the INSTALL_RPATH property on the executables to
    # "@loader_path/../${LIBDIR}"
  endif()

else()

    # Linking to setargv.obj enables wildcard globbing for the
    # command line utilities, when compiling with MSVC
    # https://docs.microsoft.com/cpp/c-language/expanding-wildcard-arguments
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} setargv.obj")

endif()

if(BUILD_CCT)
  include(bin_cct.cmake)
  set(BIN_TARGETS ${BIN_TARGETS} cct)
endif()

if(BUILD_CS2CS)
  include(bin_cs2cs.cmake)
  set(BIN_TARGETS ${BIN_TARGETS} cs2cs)
endif()

if(BUILD_GEOD)
  include(bin_geod.cmake)
  if(BUILD_TESTING)
    include(bin_geodtest.cmake)
  endif()
  set(BIN_TARGETS ${BIN_TARGETS} geod)
endif()

if(BUILD_PROJ)
  include(bin_proj.cmake)
  set(BIN_TARGETS ${BIN_TARGETS} binproj)
endif()

if(BUILD_PROJINFO)
  include(bin_projinfo.cmake)
  set(BIN_TARGETS ${BIN_TARGETS} binprojinfo)
endif()

if(BUILD_GIE)
  include(bin_gie.cmake)
  set(BIN_TARGETS ${BIN_TARGETS} gie)
endif()

if(BUILD_PROJSYNC)
  if(NOT ENABLE_CURL)
    message(SEND_ERROR "projsync requires Curl")
  endif()
  include(bin_projsync.cmake)
  set(BIN_TARGETS ${BIN_TARGETS} bin_projsync)
endif()


if (FALSE) # XXX(kitware): VTK handles this.
if(MSVC OR CMAKE_CONFIGURATION_TYPES)
  if(BIN_TARGETS)
    # Add _d suffix for your debug versions of the tools
    set_target_properties(${BIN_TARGETS} PROPERTIES
      DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
  endif()
endif()
endif ()
