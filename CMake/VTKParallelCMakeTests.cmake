# See if we need to link the socket library
INCLUDE(CheckLibraryExists)
INCLUDE(CheckSymbolExists)

CHECK_LIBRARY_EXISTS("socket" getsockname "" VTK_HAVE_LIBSOCKET)

IF(NOT DEFINED VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
  IF(VTK_HAVE_LIBSOCKET)
    SET(VTK_GETSOCKNAME_LIBS "socket")
  ELSE()
    SET(VTK_GETSOCKNAME_LIBS)
  ENDIF()
  MESSAGE(STATUS "Checking for getsockname with socklen_t")
  TRY_COMPILE(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T
    ${VTK_BINARY_DIR}/CMakeTmp/SocklenT
    ${VTK_CMAKE_DIR}/vtkTestSocklenT.cxx
    CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${VTK_GETSOCKNAME_LIBS}"
    OUTPUT_VARIABLE OUTPUT)
  IF(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
    MESSAGE(STATUS "Checking for getsockname with socklen_t -- yes")
    SET(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T 1 CACHE INTERNAL "Support for getsockname with socklen_t")
    FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if getsockname accepts socklen_t type  "
      "passed with the following output:\n"
      "${OUTPUT}\n")
  ELSE()
    MESSAGE(STATUS "Checking for getsockname with socklen_t -- no")
    SET(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T 0 CACHE INTERNAL "Support for getsockname with socklen_t")
    FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if getsockname accepts socklen_t type  "
      "failed with the following output:\n"
      "${OUTPUT}\n")
  ENDIF()
ENDIF()

# e.g. IBM BlueGene/L doesn't have SO_REUSEADDR, because "setsockopt is not needed for
# BlueGene/L applications" according to the BlueGene/L Application Development handbook
CHECK_SYMBOL_EXISTS(SO_REUSEADDR "sys/types.h;sys/socket.h" VTK_HAVE_SO_REUSEADDR)

SET(HAVE_SOCKETS TRUE)
# Cray Xt3/ Catamount doesn't have any socket support
# this could also be determined by doing something like
# check_symbol_exists(socket "sys/types.h;sys/socket.h" HAVE_SOCKETS)
#IF(CMAKE_SYSTEM MATCHES Catamount) #Xt6 and Xt7 at least have sockets, so disable this.
#  SET(HAVE_SOCKETS FALSE)
#ENDIF()

