# See if we need to link the socket library
include(CheckLibraryExists)
include(CheckSymbolExists)

check_library_exists("socket" getsockname "" VTK_HAVE_LIBSOCKET)

if(NOT DEFINED VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
  set(VTK_GETSOCKNAME_LIBS)
  if(VTK_HAVE_LIBSOCKET)
    list(APPEND VTK_GETSOCKNAME_LIBS "socket")
  endif()
  message(STATUS "Checking for getsockname with socklen_t")
  try_compile(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T_COMPILED
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/SocklenT
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkTestSocklenT.cxx
    CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${VTK_GETSOCKNAME_LIBS}"
    OUTPUT_VARIABLE OUTPUT)
  if(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T_COMPILED)
    set(vtk_getsockname_with_socklen_t_detection "success")
    set(vtk_have_getsockname_with_socklen_t 1)
  else()
    set(vtk_getsockname_with_socklen_t_detection "failed")
    set(vtk_have_getsockname_with_socklen_t 0)
  endif()
  message(STATUS "Checking for getsockname with socklen_t -- ${vtk_have_getsockname_with_socklen_t}")
  set(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T ${vtk_have_getsockname_with_socklen_t}
    CACHE INTERNAL "Support for getsockname with socklen_t")
  file(APPEND "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log"
    "Determining if getsockname accepts socklen_t type  "
    "completed with the following output:\n"
    "${OUTPUT}\n")
endif()

# e.g. IBM BlueGene/L doesn't have SO_REUSEADDR, because "setsockopt is not needed for
# BlueGene/L applications" according to the BlueGene/L Application Development handbook
check_symbol_exists(SO_REUSEADDR "sys/types.h;sys/socket.h" VTK_HAVE_SO_REUSEADDR)
