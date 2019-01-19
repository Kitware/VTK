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
    ${TK_INCLUDE_PATH})
