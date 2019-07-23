set(headers
  loguru.hpp
  "${CMAKE_CURRENT_BINARY_DIR}/vtkloguru_export.h")

set(sources
  loguru.cpp)

vtk_module_add_module(VTK::loguru
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkloguru")

include(GenerateExportHeader)
generate_export_header(loguru
  EXPORT_MACRO_NAME LOGURU_EXPORT
  EXPORT_FILE_NAME vtkloguru_export.h)

vtk_module_find_package(PACKAGE Threads)
vtk_module_link(VTK::loguru PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
