#
# Config.mk --
#
#              READ THIS FIRST: FILE PATH SPECIFICATION RULES.
#------------------------------------------------------------------------------
# All paths to files outside of the distribution MUST be fully specified 
# paths (starting with /), e.g.  /usr/local/lib/libtcl.a
#
# Fully specified paths to the Tcl and Tk librarys (libtcl.a/libtk.a) 
# and the Tcl/Tk include files.  You must edit these lines to point
# to the correct locations for your tcl/tk installations

TCL_INCLUDE=/common/software/tcl7.5/include
TCL_LIB=/common/software/tcl7.5/sun4/5.4/lib/libtcl7.5.a

TK_INCLUDE=/common/software/tk4.1/include
TK_LIB=/common/software/tk4.1/sun4/5.4/lib/libtk4.1.a

MESA_INCLUDE=/home/martink/storage/Mesa-1.2.6/include
MESA_LIB=/home/martink/storage/Mesa-1.2.6/lib-sun4-solaris/libMesaGL.a

#------------------------------------------------------------------------------
# C compiler and debug/optimization/profiling flag to use.  Set by configure,
# and are normally overridden on the make command line (make CFLAGS=-g).  
# They can also be overridden here.
#
# WARNING - if you specify -g the executables and libraries can get pretty
#           big. You better really want that debug info.
# 
# To change what compiler is used you should do something like
#   setenv CC /my/compiler/cc
#   setenv CXX /my/compiler/c++
#
# Then run the configure script. If you are not using a csh syntax then try
#   set CC=/my/compiler/cc
#   export CC
#   set CXX=/my/compiler/c++
#   export CXX 
# instead.


# here are some common settings for gcc, g++
#
CFLAGS=-O2 -Wall
CXXFLAGS=-O2 -Wall
MAKE_DEPEND_COMMAND=${CXX} -MM  $(CPPFLAGS) $(CXX_FLAGS) $(srcdir)/*.cxx > depend.make



