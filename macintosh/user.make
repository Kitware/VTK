# this file can be used to override any of the values selected
# by configure in system.make.  At a minimum you should set
# the following Tcl/Tk values if you are planning to use Tcl/Tk

# for python you must set this 
PYTHON_INCLUDES=-I/usr/local/include/python2.1
PYTHON_LIB=

# Add additional CFLAGS and CXXFLAGS for compilation
# uncomment the following two lines to set your own flags
USER_CFLAGS = -DDARWIN -dynamic -O2 -traditional-cpp
USER_CXXFLAGS = -DDARWIN -dynamic -O2  -traditional-cpp 

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






