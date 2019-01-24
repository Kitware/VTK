# TODO: Make FindTCL.cmake use imported targets
find_package(TCL REQUIRED)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkTkAppInitConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkTkAppInitConfigure.h")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkTkInternals.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h")

set(sources
  vtkPythonTkWidgetsInit.cxx
  vtkTkImageViewerWidgetPython.cxx
  vtkTkRenderWidgetPython.cxx)
set(private_headers
  vtkTcl.h
  vtkTclUtil.h
  vtkTk.h
  vtkTkImageViewerWidget.h
  vtkTkRenderWidget.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkTkAppInitConfigure.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h")

if (VTK_USE_COCOA)
  list(APPEND sources
    vtkCocoaTkUtilities.mm)
  list(APPEND private_headers
    vtkCocoaTkUtilities.h)
endif ()

# Need Tk internal headers on Windows and OS X
set(_internal_tk_header)
set(_internal_tk_include_paths)
if (WIN32)
  get_filename_component(_parent "${TK_INCLUDE_PATH}" PATH)
  set(_internal_tk_header "tkWinInt.h")
  set(_internal_tk_include_paths "${_parent}/win")
elseif (VTK_USE_COCOA)
  get_filename_component(_parent "${TK_INCLUDE_PATH}" PATH)
  set(_internal_tk_header "tkMacOSXInt.h")
  set(_internal_tk_include_paths
    "${_parent}/PrivateHeaders"
    "${_parent}/tk-private"
    "${_parent}/macosx")
endif ()

if (_internal_tk_header)
  find_path(
     TK_INTERNAL_PATH
     ${_internal_tk_header}
     PATHS ${_internal_tk_include_paths} ${TK_INCLUDE_PATH}
     DOC "The path to the Tk internal headers (${_internal_tk_header}).")
  mark_as_advanced(TK_INTERNAL_PATH)
else ()
  set(TK_INTERNAL_PATH)
endif ()

vtk_module_add_module(VTK::RenderingTk
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
vtk_module_link(VTK::RenderingTk
  PRIVATE
    ${TCL_LIBRARY}
    ${TK_LIBRARY})
vtk_module_include(VTK::RenderingTk
  PRIVATE
    ${TCL_INCLUDE_PATH}
    ${TK_INCLUDE_PATH}
    ${TK_INTERNAL_PATH})
