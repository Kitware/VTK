# this file can be used to override any of the values selected
# by configure in system.make.  At a minimum you should set
# the following Tcl/Tk values if you are planning to use Tcl/Tk

# The configure script should find TCL and TK properly, if not,
# edit these lines to point to the proper places
TCL_INCLUDE=
TCL_LIB=-ltcl

TK_INCLUDE=
TK_LIB=-ltk

# If Mesa is not installed in a default location, edit these lines.
MESA_INCLUDE=
MESA_LIB=-lMesaGL

# for python you must set this, this is the default for Python 1.5
PYTHON_INCLUDES=-I/usr/include/python1.5

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

# For newer versions of JDK, you can set things up like this:

# JDKHOME=/usr/local/jdk1.2.2/
# JAVABIN = ${JDKHOME}bin/
# JAVAC = ${JAVABIN}javac
# JAR = ${JAVABIN}jar
# JAVA_CLASS_HOME=../java/ -classpath ../java/
# JAVAH=${JDKHOME}/bin/javah
# JAVA_INCLUDES=-I${JDKHOME}include -I${JDKHOME}include/linux
# JAVA_CXX_LIB=






