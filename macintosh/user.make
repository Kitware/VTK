# this file can be used to override any of the values selected
# by configure in system.make.  At a minimum you should set
# the following Tcl/Tk values if you are planning to use Tcl/Tk

# for python you must set this 
PYTHON_INCLUDES=-I/usr/local/include/python2.1
PYTHON_LIB=

# Add additional CFLAGS and CXXFLAGS for compilation
# uncomment the following two lines to set your own flags
#USER_CFLAGS = -DDARWIN -O3 -fomit-frame-pointer -fno-schedule-insns -fschedule-insns2 -mcpu=750 -force_cpusubtype_ALL
#USER_CXXFLAGS = -DDARWIN -O3 -fomit-frame-pointer -fno-schedule-insns -fschedule-insns2 -mcpu=750 -force_cpusubtype_ALL
USER_CFLAGS = -DDARWIN -O
USER_CXXFLAGS = -DDARWIN -O


# if you want to try the java support you'll need to set the following
# variables to match your environment and uncomment them
#
JDKHOME=/System/Library/Frameworks/JavaVM.framework/Home
JAVAC=${JDKHOME}/bin/javac -classpath $(VTK_OBJ)/java
JAR=${JDKHOME}/bin/jar
JAVA_CLASS_HOME=$(VTK_OBJ)/java
JAVAH=${JDKHOME}/bin/javah
JAVA_INCLUDES=-I/System/Library/Frameworks/JavaVM.framework/Headers





