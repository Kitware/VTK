# this file can be used to override any of the values selected
# by configure in system.make.  At a minimum you should set
# the following Tcl/Tk values if you are planning to use Tcl/Tk

TCL_INCLUDE=-I/home/ausable/software/src/tcl7.6/tcl7.6/generic
TCL_LIB=/common/software/tcl7.6/sun4/5.4/lib/libtcl7.6.a

TK_INCLUDE=-I/home/ausable/software/src/tcl7.6/tk4.2/generic
TK_LIB=/common/software/tk4.2/sun4/5.4/lib/libtk4.2.a

MESA_INCLUDE=-I/home/martink/storage/Mesa-1.2.6/include
MESA_LIB=/home/martink/storage/Mesa-1.2.6/lib-sun4-solaris/libMesaGL.a

# for python you must set this 
PYTHON_INCLUDES=-I/home/schroede/montnaro/Python-1.4/Include

# Add additional CFLAGS and CXXFLAGS for compilation
# uncomment the following two lines to set your own flags
#USER_CFLAGS =  
#USER_CXXFLAGS = 

# if you want to try the java support you'll need to set the following
# variables to match your environment and uncomment them
#
#JDKHOME=/home/calvin/content/ITL/java-packages/dev-kits/JDK/jdk1.1
#JAVAC=${JDKHOME}/bin/javac
#JAR=${JDKHOME}/bin/jar
#JAVA_CLASS_HOME=/home/martink/java
#JAVAH=${JDKHOME}/bin/javah
#JAVA_INCLUDES=-I${JDKHOME}/include -I${JDKHOME}/include/solaris
#JAVA_CXX_LIB=/common/software/g++-2.7.1/sun4/5.4/lib/libiberty.a /common/software/g++-2.7.1/sun4/5.4/lib/libstdc++.a /common/software/g++-2.7.1/sun4/5.4/lib/gcc-lib/sparc-sun-solaris2.4/2.7.1/libgcc.a






