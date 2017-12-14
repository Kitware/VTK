set(Module_SRCS
  vtkClientSocket.cxx
  vtkDirectory.cxx
  vtkServerSocket.cxx
  vtkSocket.cxx
  vtkSocketCollection.cxx
  vtkThreadMessager.cxx
  vtkTimerLog.cxx
  )

include("${CMAKE_CURRENT_SOURCE_DIR}/vtkTestSocketFeatures.cmake")

set(vtkSocket_defines)
if (VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
  list(APPEND vtkSocket_defines "VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T")
endif ()
if (VTK_HAVE_SO_REUSEADDR)
  list(APPEND vtkSocket_defines "VTK_HAVE_SO_REUSEADDR")
endif ()
if(VTK_NO_PLATFORM_SOCKETS)
  list(APPEND vtkSocket_defines "VTK_SOCKET_FAKE_API")
endif()

set_source_files_properties(vtkSocket.cxx
  PROPERTIES
    COMPILE_DEFINITIONS "${vtkSocket_defines}")

if(WIN32 AND NOT BORLAND)
  set(KIT_LIBS ${KIT_LIBS} wsock32)
endif()

vtk_module_library(vtkCommonSystem ${Module_SRCS})

vtk_module_link_libraries(vtkCommonSystem LINK_PRIVATE ${CMAKE_THREAD_LIBS})
