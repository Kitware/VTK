PROJECT(vtkExodus2)

INCLUDE_REGULAR_EXPRESSION(".*")

SET(EXODUSII_VERSION_MAJOR "2")
SET(EXODUSII_VERSION_MINOR "0")
SET(EXODUSII_VERSION_PATCH "0")
SET(EXODUSII_VERSION "${EXODUSII_VERSION_MAJOR}.${EXODUSII_VERSION_MINOR}")
SET(EXODUSII_VERSION_FULL "${EXODUSII_VERSION}.${EXODUSII_VERSION_PATCH}")

SET(EXODUSII_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/exodusII_cfg.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/exodusII_cfg.h
  @ONLY IMMEDIATE)

INCLUDE_DIRECTORIES(
"${CMAKE_CURRENT_SOURCE_DIR}/include"
"${CMAKE_CURRENT_BINARY_DIR}/include"
)

SET(cbind_SRCS
  exerr.c exopts.c expini.c exgini.c excre.c
  exgcor.c expcor.c expcon.c exgcon.c expmap.c exgmap.c
  expelb.c exgelb.c expelc.c exgelc.c expeat.c
  exgeat.c expnp.c exgnp.c expns.c expnsd.c exgns.c exgnsd.c
  expcns.c exgcns.c expsp.c exgsp.c expss.c exgss.c
  expssd.c exgssd.c expcss.c exgcss.c expvpc.c
  expqa.c exgqa.c expinf.c exginf.c expvp.c exgvp.c
  expvan.c exgvan.c expvnm.c exgvnm.c expvtt.c exgvtt.c
  expgv.c exggv.c exggvt.c expnv.c exgnv.c exgnvt.c
  expev.c exgev.c exgevt.c expclb.c exgssc.c expfrm.c exgfrm.c
  exptim.c exgtim.c exgatm.c exinq.c
  exopen.c exupda.c exclos.c exgebi.c exgssi.c exgnsi.c
  ex_utils.c ex_conv.c excn2s.c exgsnl.c exgssn.c 
  exgp.c exgpa.c exgpn.c expp.c exppa.c exppn.c 
  exgenm.c exgnnm.c expnnm.c expenm.c excopy.c 
  expmp.c exgmp.c expem.c exgem.c expnm.c exgnm.c
  exgvv.c expvv.c expnvv.c exgnvv.c exgnvid.c exgevid.c
  )

IF(APPLE)
  SET_SOURCE_FILES_PROPERTIES(
    ex_utils.c
    PROPERTIES
    COMPILE_FLAGS -fno-common)
ENDIF(APPLE)


ADD_LIBRARY(vtkexoIIc ${cbind_SRCS})
TARGET_LINK_LIBRARIES(vtkexoIIc vtkNetCDF)

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(vtkexoIIc PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
    IF(VTK_INSTALL_HAS_CMAKE_24)
      INSTALL(TARGETS vtkexoIIc
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
        LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
        ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    ELSE(VTK_INSTALL_HAS_CMAKE_24)
      INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtkexoIIc)
    ENDIF(VTK_INSTALL_HAS_CMAKE_24)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
#  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}/vtkexodus2 .h
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
