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

TCL_INCLUDE=-I/common/software/tcl7.5/include
TCL_LIB=/common/software/tcl7.5/sun4/5.4/lib/libtcl7.5.a

TK_INCLUDE=-I/common/software/tk4.1/include
TK_LIB=/common/software/tk4.1/sun4/5.4/lib/libtk4.1.a

MESA_INCLUDE=-I/home/martink/storage/Mesa-1.2.6/include
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

# if you want to try the very alpha java support you'll need to set the following
#
#JAVAC=/home/martink/JDK/java/bin/javac
#JAVA_CLASS_HOME=/home/martink/java
#JAVAH=/home/martink/JDK/java/bin/javah
#JAVA_INCLUDES=-I/home/martink/JDK/java/include -I/home/martink/JDK/java/include/solaris
#JAVA_CXX_LIB=/common/software/g++-2.7.1/sun4/5.4/lib/libiberty.a /common/software/g++-2.7.1/sun4/5.4/lib/libstdc++.a /common/software/g++-2.7.1/sun4/5.4/lib/gcc-lib/sparc-sun-solaris2.4/2.7.1/libgcc.a

# here are some generic settings
#
CFLAGS=-O
CXXFLAGS=-O

# here are some common settings for gcc, g++
#
#CFLAGS=-O2 -Wall ${JAVA_INCLUDES}
#CXXFLAGS=-O2 -Wall ${JAVA_INCLUDES}
#MAKE_DEPEND_COMMAND=${CXX} -MM  $(CPPFLAGS) $(CXX_FLAGS) $(srcdir)/*.cxx > depend.make; ${CXX} -MM $(CPPFLAGS) $(CXX_FLAGS) tcl/*.cxx | sed -e "sz^\([^.]*\)\.oztcl/\1\.ozg" >> depend.make




