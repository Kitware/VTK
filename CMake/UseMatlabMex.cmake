
# This module is used for compiling with the Matlab mex interface to external C and C++
# code into Matlab as a Matlab function. 
# Defines the following:
#  ADD_MATLAB_MEX_FILE(mexfilename <source files>)
#  Creates a Matlab mex file named <mexfilename> from the given source files
#  The Matlab mex function interface mexFunction() must be defined in one of
#  the source files.

INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})
LINK_DIRECTORIES(${MATLAB_LIB_DIR})

IF(UNIX)
  SET(MEX_LIBRARIES mx mex mat m stdc++)
  SET(MATLAB_ENGINE_LIBRARIES mx eng)
  SET(MEX_VER_FILE "${MATLAB_ROOT_DIR}/extern/src/mexversion.c")
  IF(APPLE)
   IF(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bit
	SET(MEX_CXX_FLAGS "-fno-common -no-cpp-precomp -fexceptions -arch i386")
    SET(MEX_DEFINES "MATLAB_MEX_FILE")
    SET(MEX_RPATH "")
    SET(MEX_LDFLAGS "-Wl,-exported_symbols_list,${MATLAB_ROOT_DIR}/extern/lib/${MATLAB_DIR_PREFIX}/mexFunction.map")
    SET(MEX_FILE_SUFFIX ".mexmaci")
   ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4) # 64 bit
   	SET(MEX_CXX_FLAGS "-fno-common -no-cpp-precomp -fexceptions -arch x86_64")
    SET(MEX_DEFINES "MATLAB_MEX_FILE")
    SET(MEX_RPATH "")
    SET(MEX_LDFLAGS "-Wl,-exported_symbols_list,${MATLAB_ROOT_DIR}/extern/lib/${MATLAB_DIR_PREFIX}/mexFunction.map")
    SET(MEX_FILE_SUFFIX ".mexmaci64")
   ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  ELSE(APPLE)
   IF(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bit
    SET(MEX_CXX_FLAGS "-ansi -fPIC -pthread")
    SET(MEX_DEFINES "MATLAB_MEX_FILE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE")
    SET(MEX_RPATH "-Wl,-rpath-link,${MATLAB_LIB_DIR}")
    SET(MEX_LDFLAGS "-pthread -shared -m32 -Wl,--version-script,${MATLAB_ROOT_DIR}/extern/lib/${MATLAB_DIR_PREFIX}/mexFunction.map -Wl,--no-undefined")
    SET(MEX_FILE_SUFFIX ".mexglx")
   ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4) # 64 bit
    SET(MEX_CXX_FLAGS "-ansi -fPIC -pthread -fno-omit-frame-pointer")
    SET(MEX_DEFINES "MATLAB_MEX_FILE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE")
    SET(MEX_RPATH "-Wl,-rpath-link,${MATLAB_LIB_DIR}")
    SET(MEX_LDFLAGS "-pthread -shared -Wl,--version-script,${MATLAB_ROOT_DIR}/extern/lib/${MATLAB_DIR_PREFIX}/mexFunction.map -Wl,--no-undefined")
    SET(MEX_FILE_SUFFIX ".mexa64")
   ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  ENDIF(APPLE)
ELSE(UNIX)
  SET(MEX_LIBRARIES libmx libmex libmat)
  SET(MATLAB_ENGINE_LIBRARIES libmx libeng)
  SET(MEX_VER_FILE "")
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bit
   SET(MEX_CXX_FLAGS "")
   SET(MEX_DEFINES "MATLAB_MEX_FILE")
   SET(MEX_RPATH "")
   SET(MEX_LDFLAGS "/MAP /export:mexFunction")
   SET(MEX_FILE_SUFFIX ".mexw32")  
  ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4) # 64 bit
   SET(MEX_CXX_FLAGS "")
   SET(MEX_DEFINES "MATLAB_MEX_FILE")
   SET(MEX_RPATH "")
   SET(MEX_LDFLAGS "/MAP /export:mexFunction")
   SET(MEX_FILE_SUFFIX ".mexw64")  
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
ENDIF(UNIX)

MACRO(ADD_MATLAB_MEX_FILE mexfilename)
  ADD_LIBRARY(${mexfilename} SHARED ${ARGN} ${MEX_VER_FILE})
  TARGET_LINK_LIBRARIES(${mexfilename} ${MEX_LIBRARIES} vtksnlInfovisMatlabEngine)
  SET_TARGET_PROPERTIES(${mexfilename} PROPERTIES 
                        PREFIX "" 
                        SUFFIX "${MEX_FILE_SUFFIX}"
                        COMPILE_FLAGS "${MEX_CXX_FLAGS}"
                        DEFINE_SYMBOL "${MEX_DEFINES} ${DEFINE_SYMBOL}"
                        LINK_FLAGS "${MEX_RPATH} ${MEX_LDFLAGS} ${LINK_FLAGS}")
ENDMACRO(ADD_MATLAB_MEX_FILE)

