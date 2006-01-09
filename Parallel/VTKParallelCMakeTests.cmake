# See if we need to link the socket library 
INCLUDE(${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)
CHECK_LIBRARY_EXISTS("socket" getsockname "" VTK_HAVE_LIBSOCKET)

IF("VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T" MATCHES "^VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T$")
  IF(VTK_HAVE_LIBSOCKET)
    SET(VTK_GETSOCKNAME_LIBS "socket")
  ELSE(VTK_HAVE_LIBSOCKET)
    SET(VTK_GETSOCKNAME_LIBS)
  ENDIF(VTK_HAVE_LIBSOCKET)
  MESSAGE(STATUS "Checking for getsockname with socklen_t")
  TRY_COMPILE(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T
    ${VTK_BINARY_DIR}/CMakeTmp/SocklenT
    ${VTK_CMAKE_DIR}/vtkTestSocklenT.cxx
    CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${VTK_GETSOCKNAME_LIBS}"
    OUTPUT_VARIABLE OUTPUT)
  IF(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
    MESSAGE(STATUS "Checking for getsockname with socklen_t -- yes")
    SET(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T 1 CACHE INTERNAL "Support for getsockname with socklen_t")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if getsockname accepts socklen_t type  "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
    MESSAGE(STATUS "Checking for getsockname with socklen_t -- no")
    SET(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T 0 CACHE INTERNAL "Support for getsockname with socklen_t")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if getsockname accepts socklen_t type  "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
ENDIF("VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T" MATCHES "^VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T$")
