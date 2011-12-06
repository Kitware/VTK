## This is a automake file, part of Unidata's netCDF package.
# Copyright 2005, see the COPYRIGHT file for more information.

# This automake file generates the Makefile to build netCDF-4. The
# generated makefile is not run unless the user selected to build
# netCDF-4.

include $(top_srcdir)/lib_flags.am

# Initialize
LDADD =
libnetcdf4_la_CPPFLAGS = ${AM_CPPFLAGS}

# Netcdf-4 source.
NC4SOURCES = nc4internal.h error4.c nc4dispatch.c nc4dispatch.h		\
nc4attr.c nc4dim.c nc4file.c nc4grp.c nc4type.c nc4var.c ncfunc.c	\
nc4internal.c nc4hdf.c

# This is our output. The big enchilada.
noinst_LTLIBRARIES = libnetcdf4.la
libnetcdf4_la_SOURCES = ${NC4SOURCES}

# Tell libtool what libraries are we will depend on.
libnetcdf4_la_LIBADD =

# HDF4 used?
if USE_HDF4
libnetcdf4_la_LIBADD += -lmfhdf -ldf
endif # USE_HDF4

# How about pnetcdf?
if USE_PNETCDF
libnetcdf4_la_LIBADD += -lpnetcdf
endif # USE_PNETCDF

# Always link to hdf5_hl, hdf5, and z.
libnetcdf4_la_LIBADD += -lhdf5_hl -lhdf5 -lz

# This turns on declspec magic in netcdf.h for windows DLLs.
if BUILD_DLL
libnetcdf4_la_CPPFLAGS += -DDLL_EXPORT
endif
