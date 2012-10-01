#------------------------------------------------------------------------------
# libproj4 -- library of cartographic projections
#
project( libproj4 )

#------------------------------------------------------------------------------
# Platform configuration tests and options.

# Shared or static library?
if ( NOT DEFINED BUILD_SHARED_LIBS )
  option( BUILD_SHARED_LIBS "Should libproj4 be built as a shared library (or static)?" OFF )
endif ( NOT DEFINED BUILD_SHARED_LIBS )

# Only experts should turn this on:
option( PROJ_LIST_EXTERNAL "Does an external project define proj_list or should libproj4 define it?" OFF )
mark_as_advanced( PROJ_LIST_EXTERNAL )
if ( PROJ_LIST_EXTERNAL AND BUILD_SHARED_LIBS )
  message( FATAL_ERROR "You may not define both PROJ_LIST_EXTERNAL and BUILD_SHARED_LIBS. Turn one off and re-run CMake." )
endif ( PROJ_LIST_EXTERNAL AND BUILD_SHARED_LIBS )

# Make libproj4 thread-safe?
option( PROJ_USE_PTHREADS "Should libproj4 be built as a thread-friendly library?" OFF )
mark_as_advanced( PROJ_USE_PTHREADS )
if ( PROJ_USE_PTHREADS )
  if ( NOT VTK_USE_PTHREADS )
    message( FATAL "Your platform does not appear to support pthreads. Please turn PROJ_USE_PTHREADS off and retry." )
  endif ( NOT VTK_USE_PTHREADS )
endif ( PROJ_USE_PTHREADS )

# Include code that requires GSL
option( PROJ_USE_GSL "Should libproj4 include projection code that relies on GSL?" OFF )
mark_as_advanced( PROJ_USE_GSL )
if ( PROJ_USE_GSL )
  find_package( PkgConfig )
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules( GSL REQUIRED gsl )
  endif ( PKG_CONFIG_FOUND )
  if ( NOT GSL_FOUND )
    message( FATAL_ERROR "You must install GSL and its pkg-config file OR turn PROJ_USE_GSL off" )
  endif ( NOT GSL_FOUND )
endif ( PROJ_USE_GSL )

# Test for "double complex" type and "atanh()" support.
if ( UNIX )
  set( LINK_LIBRARIES m )
endif ( UNIX )

include(CheckSymbolExists)
set(CMAKE_REQUIRED_LIBRARIES m)
check_symbol_exists(csin complex.h PROJ_HAVE_COMPLEX)
check_symbol_exists(atanh math.h PROJ_HAVE_ATANH)
set(CMAKE_REQUIRED_LIBRARIES)

#------------------------------------------------------------------------------
# Source
set( AZIM_SRCS
  proj_aeqd.c proj_aitoff.c proj_geos.c
  proj_gilbert.c proj_gnom.c proj_hammer.c
  proj_laea.c proj_mod_ster.c proj_nsper.c
  proj_nzmg.c proj_ortho.c proj_rouss.c
  proj_sterea.c proj_stere.c proj_wag7.c
  proj_wag9.c
)

set( CONIC_SRCS
  proj_aea.c proj_bipc.c proj_bonne.c
  proj_eqdc.c proj_hill.c proj_imw_p.c
  proj_kocc.c proj_lcca.c proj_lcc.c
  proj_poly.c proj_rpoly.c proj_sconics.c
)

set( CYLIND_SRCS
  proj_ardn_cls.c proj_braun2.c proj_cass.c
  proj_cc.c proj_cea.c proj_cubic.c
  proj_cyl_stere.c proj_eqc.c proj_labrd.c
  proj_lsat.c proj_merc.c proj_mill_2.c
  proj_mill.c proj_mill_per.c proj_ocea.c
  proj_omerc.c proj_somerc.c proj_tcc.c
  proj_tcea.c proj_tmerc.c proj_tob_sqr.c
  proj_etmerc.c
)

if ( PROJ_HAVE_COMPLEX )
  set( CYLIND_SRCS
    ${CYLIND_SRCS}
    proj_ftmerc.c
  )
endif ( PROJ_HAVE_COMPLEX )

if ( PROJ_HAVE_ATANH )
  set( CYLIND_SRCS
    ${CYLIND_SRCS}
    proj_gstmerc.c
    proj_ktmerc.c
  )
endif ( PROJ_HAVE_ATANH )

set( MISC_SRCS
  proj_airy.c proj_apian2.c proj_armadillo.c
  proj_august.c proj_bacon.c
  proj_eisen.c proj_four1.c proj_guyou.c
  proj_lagrng.c proj_larr.c proj_lask.c
  proj_nocol.c proj_ob_tran.c proj_oea.c
  proj_tpeqd.c proj_vandg2.c proj_vandg4.c
  proj_vandg.c
)

set( PSEUDO_SRCS
  proj_baker.c proj_baranyi4.c proj_baranyi.c
  proj_boggs.c proj_collg.c proj_crast.c
  proj_denoy.c proj_eck1.c proj_eck2.c
  proj_eck3.c proj_eck4.c proj_eck5.c
  proj_equi_moll.c proj_erdi_ksz.c proj_fahey.c
  proj_fc-loerr.c proj_fouc_s.c proj_four2.c
  proj_gins8.c proj_gn_sinu.c proj_goode.c
  proj_hatano.c proj_holzel.c proj_loxim.c
  proj_maurer.c proj_mayr.c proj_mbtfpp.c
  proj_mbtfpq.c proj_mbt_fps.c proj_mcb_s3.c
  proj_moll.c proj_nell.c
  proj_oxford.c proj_putp2.c proj_putp3.c
  proj_putp4p.c proj_putp5.c proj_putp6.c
  proj_robin.c proj_semiconf.c proj_s_min_err.c
  proj_sts.c proj_tob_g1.c proj_trapez.c
  proj_urm5.c proj_urmfps.c proj_wag2.c
  proj_wag3.c proj_wag5.c proj_wink1.c
  proj_wink2.c
)

if ( PROJ_HAVE_ATANH )
  set( PSEUDO_SRCS
    ${PSEUDO_SRCS}
    proj_nell_h.c
  )
endif ( PROJ_HAVE_ATANH )

set( SUPPORT_SRCS
  proj_adjlon.c proj_auth.c proj_deriv.c
  proj_dmstor.c proj_ellps.c proj_ell_set.c
  proj_err_list.c proj_errno.c proj_factors.c
  proj_fwd.c proj_gauss.c proj_init.c
  proj_inv.c proj_mdist.c
  proj_msfn.c proj_param.c proj_phi2.c
  proj_pr_list.c proj_rtodms.c proj_strerrno.c
  proj_strerror_r.c proj_trans.c proj_translate.c
  proj_tsfn.c proj_units.c proj_zpoly1.c
  proj_psi.c
)
if ( NOT PROJ_LIST_EXTERNAL )
  set( SUPPORT_SRCS
    ${SUPPORT_SRCS}
    proj_list.c
  )

#------------------------------------------------------------------------------
# Doesn't work with CMake 2.4
#  set_source_files_properties(
#    FILES
#      proj_list.c
#    PROPERTIES
#      OBJECT_DEPENDS ${libproj4_SOURCE_DIR}/proj_list.h
#  )
endif ( NOT PROJ_LIST_EXTERNAL )

set( LP4_SRCS
  ${AZIM_SRCS} ${CONIC_SRCS} ${CYLIND_SRCS}
  ${MISC_SRCS} ${PSEUDO_SRCS} ${SUPPORT_SRCS}
)

#------------------------------------------------------------------------------
# Executable and library targets

vtk_add_library( vtkproj4
  ${LP4_SRCS}
)

if ( UNIX )
  target_link_libraries( vtkproj4
    m
  )
endif ( UNIX )

if ( PROJ_LIBRARY_PROPERTIES )
  set_target_properties( vtkproj4
    PROPERTIES
      ${PROJ_LIBRARY_PROPERTIES}
  )
endif ( PROJ_LIBRARY_PROPERTIES )

add_executable( lproj
  lproj.c
)

target_link_libraries( lproj
  vtkproj4
)

include_directories(
  ${libproj4_SOURCE_DIR}
  ${libproj4_BINARY_DIR}
)

if ( GSL_FOUND )
  include_directories( GSL_INCLUDE_DIRS )
  target_link_libraries( proj4
    ${GSL_LIBRARIES}
  )
endif ( GSL_FOUND )

add_custom_command(TARGET lproj
  COMMAND ${CMAKE_COMMAND} -E copy
  ${libproj4_SOURCE_DIR}/vtk_libproj4_mangle.h
  ${libproj4_BINARY_DIR}/vtk_libproj4_mangle.h)

configure_file(
  ${libproj4_SOURCE_DIR}/proj_config.h.cmake
  ${libproj4_BINARY_DIR}/proj_config.h
)

#------------------------------------------------------------------------------
# Install targets

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${libproj4_SOURCE_DIR}/lib_proj.h
    ${libproj4_SOURCE_DIR}/vtk_libproj4_mangle.h
    ${libproj4_BINARY_DIR}/proj_config.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtklibproj4
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
