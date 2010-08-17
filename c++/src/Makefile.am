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
INCLUDES=-I$(top_srcdir)/src

# This is our main target
lib_LTLIBRARIES=libhdf5_cpp.la

# Add libtool numbers to the HDF5 C++ library (from config/lt_vers.am)
libhdf5_cpp_la_LDFLAGS= -version-info $(LT_VERS_INTERFACE):$(LT_VERS_REVISION):$(LT_VERS_AGE) $(AM_LDFLAGS)

# Shared C++ libraries aren't universally supported.
if CXX_SHARED_CONDITIONAL
else
  AM_LDFLAGS+=-static
endif

bin_SCRIPTS=h5c++

# Source files for the library
libhdf5_cpp_la_SOURCES=H5Exception.cpp H5IdComponent.cpp H5Library.cpp        \
        H5Attribute.cpp H5Object.cpp H5PropList.cpp H5FaccProp.cpp            \
        H5FcreatProp.cpp H5DcreatProp.cpp H5DxferProp.cpp H5DataType.cpp      \
        H5DataSpace.cpp H5AbstractDs.cpp H5AtomType.cpp H5PredType.cpp        \
        H5EnumType.cpp H5IntType.cpp H5FloatType.cpp H5StrType.cpp            \
        H5ArrayType.cpp H5VarLenType.cpp H5CompType.cpp H5DataSet.cpp         \
        H5CommonFG.cpp H5Group.cpp H5File.cpp

# Public headers
include_HEADERS=H5Cpp.h H5AbstractDs.h H5AtomType.h H5Attribute.h H5Classes.h \
        H5CommonFG.h H5CompType.h H5DataSet.h H5DataSpace.h H5DataType.h      \
        H5DcreatProp.h H5DxferProp.h H5EnumType.h H5Exception.h H5FaccProp.h  \
        H5FcreatProp.h H5File.h H5FloatType.h H5Group.h H5IdComponent.h       \
        H5Include.h H5IntType.h H5Library.h H5Object.h H5PredType.h           \
        H5PropList.h H5StrType.h H5CppDoc.h H5ArrayType.h H5VarLenType.h      

# h5c++ and libhdf5.settings are generated during configure.  Remove only when
# distclean.
DISTCLEANFILES=h5c++ libhdf5.settings

# Some C++ compilers/linkers (PGI?) create a directory named "ii_files" that
# holds *.ii files, which are template entity instantiations.
# This entire directory should be cleaned.
mostlyclean-local:
	@if test -d ii_files; then                                       \
	  $(RM) -rf ii_files;                                           \
	fi

# Mark this directory as part of the C++ API
CXX_API=yes

include $(top_srcdir)/config/conclude.am
