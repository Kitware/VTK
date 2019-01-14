vtk_module_find_package(
  PACKAGE Threads)

set(classes
  vtkClientSocket
  vtkDirectory
  vtkServerSocket
  vtkSocket
  vtkSocketCollection
  vtkThreadMessager
  vtkTimerLog)

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

vtk_module_add_module(VTK::CommonSystem
  CLASSES ${classes})
vtk_module_link(VTK::CommonSystem
  PRIVATE
    $<$<PLATFORM_ID:WIN32>:wsock32>
    Threads::Threads)
