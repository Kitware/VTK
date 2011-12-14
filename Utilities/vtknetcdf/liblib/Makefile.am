## This is a automake file, part of Unidata's netCDF package.
# Copyright 2010, see the COPYRIGHT file for more information.

# This Makefile assembles the correct libnetcdf based on various
# configure flags. It is assumed that all the relevant convenience
# libraries have been built (e.g. libsrc, libsrc4, libncdap3, libcdmr,
# libncdap4, fortran).

AM_CPPFLAGS = -I$(top_srcdir)/include
libnetcdf_la_CPPFLAGS = ${AM_CPPFLAGS}

# This is our output, the netcdf library the user will install.
lib_LTLIBRARIES = libnetcdf.la

# These linker flags specify libtool version info.
libnetcdf_la_LDFLAGS = -version-number 7:0:1
libnetcdf_la_LIBADD =

# Turn on some extra stuff when building a DLL for windows.
if BUILD_DLL
libnetcdf_la_LDFLAGS += -no-undefined -Wl,--output-def,netcdfdll.def
libnetcdf_la_CPPFLAGS += -DDLL_EXPORT
endif # BUILD_DLL

# The v2 API...
if BUILD_V2
libnetcdf_la_LIBADD += ${top_builddir}/libdispatch/libnetcdf2.la
endif # BUILD_V2

# The output library will always include netcdf3 and dispatch libraries.
libnetcdf_la_LIBADD += ${top_builddir}/libdispatch/libdispatch.la	\
${top_builddir}/libsrc/libnetcdf3.la

# netcdf3 + dap
if USE_DAP
AM_CPPFLAGS += -I${top_srcdir}/libncdap3 -I${top_srcdir}/oc
libnetcdf_la_LIBADD += ${top_builddir}/libncdap3/libncdap3.la
libnetcdf_la_LIBADD += ${top_builddir}/oc/liboc.la
endif # USE_DAP

# NetCDF-4 ...
if USE_NETCDF4

AM_CPPFLAGS += -I${top_srcdir}/libsrc4
libnetcdf_la_LIBADD += ${top_builddir}/libsrc4/libnetcdf4.la

# netcdf4+dap
if USE_DAP
libnetcdf_la_LIBADD += ${top_builddir}/libncdap4/libncdap4.la
endif # USE_DAP

# netcdf4+cdmremote
if BUILD_CDMREMOTE
libnetcdf_la_LIBADD += ${top_builddir}/libcdmr/libnccr.la
endif # BUILD_CDMREMOTE

endif #USE_NETCDF4

if BUILD_F77
if ! BUILD_SEPARATE_FORTRAN
libnetcdf_la_LIBADD += ${top_builddir}/fortran/libnetcdff.la
endif #!BUILD_SEPARATE_FORTRAN
endif #BUILD_F77

# We need at least one source file
libnetcdf_la_SOURCES = stub.c
