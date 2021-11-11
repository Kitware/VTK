if (FALSE) # XXX(kitware): Hide configure noise.
message(STATUS "Configuring proj library:")
endif ()

##############################################
### SWITCH BETWEEN STATIC OR SHARED LIBRARY###
##############################################

if (FALSE) # XXX(kitware): Hardcode settings
# default config is shared, except static on Windows
set(BUILD_SHARED_LIBS_DEFAULT ON)
if(WIN32)
  set(BUILD_SHARED_LIBS_DEFAULT OFF)
endif()
option(BUILD_SHARED_LIBS
  "Build PROJ library shared." ${BUILD_SHARED_LIBS_DEFAULT})
endif ()

if (FALSE) # XXX(kitware): Hardcode settings
option(USE_THREAD "Build libproj with thread/mutex support " ON)
else ()
set(USE_THREAD ON)
endif ()
if(NOT USE_THREAD)
  add_definitions(-DMUTEX_stub)
endif()
find_package(Threads QUIET)
if(USE_THREAD AND Threads_FOUND AND CMAKE_USE_WIN32_THREADS_INIT)
  add_definitions(-DMUTEX_win32)
elseif(USE_THREAD AND Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  add_definitions(-DMUTEX_pthread)
elseif(USE_THREAD AND NOT Threads_FOUND)
  message(FATAL_ERROR
    "No thread library found and thread/mutex support is "
    "required by USE_THREAD option")
endif()

if (FALSE) # XXX(kitware): Hardcode settings
option(ENABLE_IPO
  "Build library with interprocedural optimization (if available)." OFF)
else ()
set(ENABLE_IPO OFF)
endif ()
if(ENABLE_IPO)
  cmake_policy(SET CMP0069 NEW)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ENABLE_IPO)
endif()
if (FALSE) # XXX(kitware): Hide configure noise.
print_variable(ENABLE_IPO)
endif ()


##############################################
###  library source list and include_list  ###
##############################################

set(SRC_LIBPROJ_PROJECTIONS
  projections/aeqd.cpp
  projections/adams.cpp
  projections/gnom.cpp
  projections/laea.cpp
  projections/mod_ster.cpp
  projections/nsper.cpp
  projections/nzmg.cpp
  projections/ortho.cpp
  projections/stere.cpp
  projections/sterea.cpp
  projections/aea.cpp
  projections/bipc.cpp
  projections/bonne.cpp
  projections/eqdc.cpp
  projections/isea.cpp
  projections/ccon.cpp
  projections/imw_p.cpp
  projections/krovak.cpp
  projections/lcc.cpp
  projections/poly.cpp
  projections/rpoly.cpp
  projections/sconics.cpp
  projections/rouss.cpp
  projections/cass.cpp
  projections/cc.cpp
  projections/cea.cpp
  projections/eqc.cpp
  projections/gall.cpp
  projections/labrd.cpp
  projections/lsat.cpp
  projections/misrsom.cpp
  projections/merc.cpp
  projections/mill.cpp
  projections/ocea.cpp
  projections/omerc.cpp
  projections/somerc.cpp
  projections/tcc.cpp
  projections/tcea.cpp
  projections/times.cpp
  projections/tmerc.cpp
  projections/tobmerc.cpp
  projections/airy.cpp
  projections/aitoff.cpp
  projections/august.cpp
  projections/bacon.cpp
  projections/bertin1953.cpp
  projections/chamb.cpp
  projections/hammer.cpp
  projections/lagrng.cpp
  projections/larr.cpp
  projections/lask.cpp
  projections/latlong.cpp
  projections/nicol.cpp
  projections/ob_tran.cpp
  projections/oea.cpp
  projections/tpeqd.cpp
  projections/vandg.cpp
  projections/vandg2.cpp
  projections/vandg4.cpp
  projections/wag7.cpp
  projections/lcca.cpp
  projections/geos.cpp
  projections/boggs.cpp
  projections/collg.cpp
  projections/comill.cpp
  projections/crast.cpp
  projections/denoy.cpp
  projections/eck1.cpp
  projections/eck2.cpp
  projections/eck3.cpp
  projections/eck4.cpp
  projections/eck5.cpp
  projections/fahey.cpp
  projections/fouc_s.cpp
  projections/gins8.cpp
  projections/gstmerc.cpp
  projections/gn_sinu.cpp
  projections/goode.cpp
  projections/igh.cpp
  projections/igh_o.cpp
  projections/hatano.cpp
  projections/loxim.cpp
  projections/mbt_fps.cpp
  projections/mbtfpp.cpp
  projections/mbtfpq.cpp
  projections/moll.cpp
  projections/nell.cpp
  projections/nell_h.cpp
  projections/patterson.cpp
  projections/putp2.cpp
  projections/putp3.cpp
  projections/putp4p.cpp
  projections/putp5.cpp
  projections/putp6.cpp
  projections/qsc.cpp
  projections/robin.cpp
  projections/sch.cpp
  projections/sts.cpp
  projections/urm5.cpp
  projections/urmfps.cpp
  projections/wag2.cpp
  projections/wag3.cpp
  projections/wink1.cpp
  projections/wink2.cpp
  projections/healpix.cpp
  projections/natearth.cpp
  projections/natearth2.cpp
  projections/calcofi.cpp
  projections/eqearth.cpp
  projections/col_urban.cpp
)

set(SRC_LIBPROJ_CONVERSIONS
  conversions/axisswap.cpp
  conversions/cart.cpp
  conversions/geoc.cpp
  conversions/geocent.cpp
  conversions/noop.cpp
  conversions/topocentric.cpp
  conversions/set.cpp
  conversions/unitconvert.cpp
)

set(SRC_LIBPROJ_TRANSFORMATIONS
  transformations/affine.cpp
  transformations/deformation.cpp
  transformations/helmert.cpp
  transformations/hgridshift.cpp
  transformations/horner.cpp
  transformations/molodensky.cpp
  transformations/vgridshift.cpp
  transformations/xyzgridshift.cpp
  transformations/defmodel.cpp
  transformations/tinshift.cpp
)

set(SRC_LIBPROJ_ISO19111
  iso19111/static.cpp
  iso19111/util.cpp
  iso19111/metadata.cpp
  iso19111/common.cpp
  iso19111/crs.cpp
  iso19111/datum.cpp
  iso19111/coordinatesystem.cpp
  iso19111/io.cpp
  iso19111/internal.cpp
  iso19111/factory.cpp
  iso19111/c_api.cpp
  iso19111/operation/concatenatedoperation.cpp
  iso19111/operation/coordinateoperationfactory.cpp
  iso19111/operation/conversion.cpp
  iso19111/operation/esriparammappings.cpp
  iso19111/operation/oputils.cpp
  iso19111/operation/parammappings.cpp
  iso19111/operation/projbasedoperation.cpp
  iso19111/operation/singleoperation.cpp
  iso19111/operation/transformation.cpp
  iso19111/operation/vectorofvaluesparams.cpp
)

set(SRC_LIBPROJ_CORE
  4D_api.cpp
  aasincos.cpp
  adjlon.cpp
  auth.cpp
  ctx.cpp
  datum_set.cpp
  datums.cpp
  deriv.cpp
  dmstor.cpp
  ell_set.cpp
  ellps.cpp
  factors.cpp
  fwd.cpp
  gauss.cpp
  generic_inverse.cpp
  geodesic.c
  init.cpp
  initcache.cpp
  internal.cpp
  inv.cpp
  list.cpp
  log.cpp
  malloc.cpp
  mlfn.cpp
  msfn.cpp
  mutex.cpp
  param.cpp
  phi2.cpp
  pipeline.cpp
  pj_list.h
  pr_list.cpp
  proj_internal.h
  proj_mdist.cpp
  qsfn.cpp
  release.cpp
  rtodms.cpp
  strerrno.cpp
  strtod.cpp
  tsfn.cpp
  units.cpp
  wkt1_generated_parser.c
  wkt1_generated_parser.h
  wkt1_parser.cpp
  wkt1_parser.h
  wkt2_generated_parser.c
  wkt2_generated_parser.h
  wkt2_parser.cpp
  wkt2_parser.h
  wkt_parser.cpp
  wkt_parser.hpp
  zpoly1.cpp
  proj_json_streaming_writer.hpp
  proj_json_streaming_writer.cpp
  tracing.cpp
  grids.hpp
  grids.cpp
  filemanager.hpp
  filemanager.cpp
  networkfilemanager.cpp
  sqlite3_utils.hpp
  sqlite3_utils.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/proj_config.h
)

set(HEADERS_LIBPROJ
  proj.h
  proj_experimental.h
  proj_constants.h
  geodesic.h
  vtk_libproj_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtklibproj_export.h"
)

# Group source files for IDE source explorers (e.g. Visual Studio)
source_group("Header Files"
  FILES ${HEADERS_LIBPROJ})
source_group("Source Files\\Core"
  FILES ${SRC_LIBPROJ_CORE})
source_group("Source Files\\Conversions"
  FILES ${SRC_LIBPROJ_CONVERSIONS})
source_group("Source Files\\Projections"
  FILES ${SRC_LIBPROJ_PROJECTIONS})
source_group("Source Files\\Transformations"
  FILES ${SRC_LIBPROJ_TRANSFORMATIONS})
source_group("Source Files\\ISO19111"
  FILES ${SRC_LIBPROJ_ISO19111})

include_directories(${PROJ_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
source_group("CMake Files" FILES CMakeLists.txt)

# Embed PROJ_LIB data files location
# XXX(kitware): Pass relative directory to look for
add_definitions(-DVTK_RELATIVE_DATADIR="${DATADIR}")

#################################################
## targets: libproj and proj_config.h
#################################################
set(ALL_LIBPROJ_SOURCES
  ${SRC_LIBPROJ_CORE}
  ${SRC_LIBPROJ_CONVERSIONS}
  ${SRC_LIBPROJ_PROJECTIONS}
  ${SRC_LIBPROJ_TRANSFORMATIONS}
  ${SRC_LIBPROJ_ISO19111}
)
set(ALL_LIBPROJ_HEADERS ${HEADERS_LIBPROJ})

# Configuration for the core target "proj"
if (FALSE) # XXX(kitware): VTK handles output names.
proj_target_output_name(proj PROJ_CORE_TARGET_OUTPUT_NAME)
endif ()

if (FALSE) # XXX(kitware): use VTK's module system.
add_library(proj
  ${ALL_LIBPROJ_SOURCES}
  ${ALL_LIBPROJ_HEADERS}
  ${PROJ_RESOURCES}
)
target_compile_options(proj
  PRIVATE $<$<COMPILE_LANGUAGE:C>:${PROJ_C_WARN_FLAGS}>
  PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${PROJ_CXX_WARN_FLAGS}>
)

if(MSVC OR MINGW)
    target_compile_definitions(proj PRIVATE -DNOMINMAX)
endif()

# Tell Intel compiler to do arithmetic accurately.  This is needed to stop the
# compiler from ignoring parentheses in expressions like (a + b) + c and from
# simplifying 0.0 + x to x (which is wrong if x = -0.0).
if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
  if(MSVC)
    set(FP_PRECISE "/fp:precise")
  else()
    set(FP_PRECISE "-fp-model precise")
  endif()
  # Apply to source files that require this option
  set_source_files_properties(
    geodesic.c
    PROPERTIES COMPILE_FLAGS ${FP_PRECISE})
endif()
else ()
vtk_module_add_module(VTK::libproj
  SOURCES ${ALL_LIBPROJ_SOURCES}
  HEADERS ${ALL_LIBPROJ_HEADERS}
  HEADERS_SUBDIR "vtklibproj/src")
target_compile_definitions(libproj PRIVATE NOMINMAX)
set(PROJ_CORE_TARGET libproj)
include(GenerateExportHeader)
generate_export_header(libproj
  EXPORT_MACRO_NAME vtklibproj_EXPORT
  EXPORT_FILE_NAME  vtklibproj_export.h)
vtk_module_include(VTK::libproj
  INTERFACE
  "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtklibproj/src>")
endif ()

if(ENABLE_IPO)
  set_property(TARGET proj
    PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(libproj INTERFACE
  $<INSTALL_INTERFACE:${INCLUDEDIR}>)

if (FALSE) # XXX(kitware): Not necessary for VTK.
if(WIN32)
  set_target_properties(proj
    PROPERTIES
    VERSION "${${PROJECT_NAME}_BUILD_VERSION}"
    OUTPUT_NAME "${PROJ_CORE_TARGET_OUTPUT_NAME}"
    ARCHIVE_OUTPUT_NAME proj
    CLEAN_DIRECT_OUTPUT 1)
elseif(BUILD_FRAMEWORKS_AND_BUNDLE)
  set_target_properties(proj
    PROPERTIES
    VERSION "${${PROJECT_NAME}_BUILD_VERSION}"
    INSTALL_NAME_DIR ${PROJ_INSTALL_NAME_DIR}
    CLEAN_DIRECT_OUTPUT 1)
else()
  set_target_properties(proj
    PROPERTIES
    VERSION "${${PROJECT_NAME}_BUILD_VERSION}"
    SOVERSION "${${PROJECT_NAME}_API_VERSION}"
    CLEAN_DIRECT_OUTPUT 1)
endif()

set_target_properties(proj
  PROPERTIES
  LINKER_LANGUAGE CXX)
endif ()

##############################################
# Link properties
##############################################
set(PROJ_LIBRARIES proj)
# hack, required for test/unit
set(PROJ_LIBRARIES ${PROJ_LIBRARIES} PARENT_SCOPE)
if(UNIX)
  find_library(M_LIB m)
  mark_as_advanced(M_LIB)
  if(M_LIB)
    target_link_libraries(libproj PRIVATE -lm)
  endif()
  find_library(DL_LIB dl)
  mark_as_advanced(DL_LIB)
  if(DL_LIB)
    target_link_libraries(libproj PRIVATE -ldl)
  endif()
endif()
if(USE_THREAD AND Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(libproj PRIVATE ${CMAKE_THREAD_LIBS_INIT})
endif()

target_include_directories(libproj PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(libproj PRIVATE ${SQLITE3_LIBRARY})

if(NLOHMANN_JSON STREQUAL "external")
  target_compile_definitions(libproj PRIVATE EXTERNAL_NLOHMANN_JSON)
  target_link_libraries(libproj PRIVATE "$<BUILD_INTERFACE:nlohmann_json::nlohmann_json>")
endif()

if(TIFF_ENABLED)
  target_compile_definitions(proj PRIVATE -DTIFF_ENABLED)
  target_include_directories(proj PRIVATE ${TIFF_INCLUDE_DIR})
  target_link_libraries(proj PRIVATE ${TIFF_LIBRARY})
endif()

if(CURL_ENABLED)
  target_compile_definitions(proj PRIVATE -DCURL_ENABLED)
  target_include_directories(proj PRIVATE ${CURL_INCLUDE_DIR})
  target_link_libraries(proj
    PRIVATE
      ${CURL_LIBRARY}
      $<$<CXX_COMPILER_ID:MSVC>:ws2_32>
      $<$<CXX_COMPILER_ID:MSVC>:wldap32>
      $<$<CXX_COMPILER_ID:MSVC>:advapi32>
      $<$<CXX_COMPILER_ID:MSVC>:crypt32>
      $<$<CXX_COMPILER_ID:MSVC>:normaliz>)
endif()

if (FALSE) # XXX(kitware): Not necessary for VTK.
if(MSVC AND BUILD_SHARED_LIBS)
  target_compile_definitions(proj
    PRIVATE PROJ_MSVC_DLL_EXPORT=1)
endif()
endif ()

##############################################
# install
##############################################
if (FALSE) # XXX(kitware): VTK handles installation.
install(TARGETS proj
  EXPORT targets
  RUNTIME DESTINATION ${BINDIR}
  LIBRARY DESTINATION ${LIBDIR}
  ARCHIVE DESTINATION ${LIBDIR}
  FRAMEWORK DESTINATION ${FRAMEWORKDIR})

if(NOT BUILD_FRAMEWORKS_AND_BUNDLE)
  install(FILES ${ALL_LIBPROJ_HEADERS}
    DESTINATION ${INCLUDEDIR})
endif()
endif ()

##############################################
# Core configuration summary
##############################################
if (FALSE) # XXX(kitware): Hide configure noise.
print_variable(PROJ_CORE_TARGET_OUTPUT_NAME)
print_variable(BUILD_SHARED_LIBS)
print_variable(PROJ_LIBRARIES)
endif ()
