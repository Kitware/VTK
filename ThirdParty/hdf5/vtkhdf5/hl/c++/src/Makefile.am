#
# Copyright by The HDF Group.
# Copyright by the Board of Trustees of the University of Illinois.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the files COPYING and Copyright.html.  COPYING can be found at the root
# of the source code distribution tree; Copyright.html can be found at the
# root level of an installed copy of the electronic HDF5 document set and
# is linked from the top-level documents page.  It can also be found at
# http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have
# access to either file, you may request a copy from help@hdfgroup.org.
##
## Makefile.am
## Run automake to generate a Makefile.in from this file.
#
# HDF5-C++ Makefile(.in)
#

include $(top_srcdir)/config/commence.am
include $(top_srcdir)/config/lt_vers.am

# Include src directory
INCLUDES=-I$(top_srcdir)/src -I$(top_srcdir)/hl/src

# This is our main target
lib_LTLIBRARIES=libhdf5_hl_cpp.la

# Add libtool numbers to the HDF5 HL C++ library (from config/lt_vers.am)
libhdf5_hl_cpp_la_LDFLAGS= -version-info $(LT_VERS_INTERFACE):$(LT_VERS_REVISION):$(LT_VERS_AGE) $(AM_LDFLAGS)

# Shared C++ libraries aren't universally supported.
if CXX_SHARED_CONDITIONAL
else
  AM_LDFLAGS+=-static
endif

# Source files for the library
# At the moment, only the H5PT Packet Table has a C++ API.
libhdf5_hl_cpp_la_SOURCES=H5PacketTable.cpp

# Public headers
include_HEADERS=H5PacketTable.h

include $(top_srcdir)/config/conclude.am
