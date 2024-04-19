set(headers
  double-conversion/bignum.h
  double-conversion/cached-powers.h
  double-conversion/diy-fp.h
  double-conversion/double-conversion.h
  double-conversion/fast-dtoa.h
  double-conversion/fixed-dtoa.h
  double-conversion/ieee.h
  double-conversion/strtod.h
  double-conversion/utils.h
  "${CMAKE_CURRENT_BINARY_DIR}/double-conversion/vtkdoubleconversion_export.h")

set(sources
  double-conversion/bignum.cc
  double-conversion/bignum-dtoa.cc
  double-conversion/cached-powers.cc
  double-conversion/diy-fp.cc
  double-conversion/double-conversion.cc
  double-conversion/fast-dtoa.cc
  double-conversion/fixed-dtoa.cc
  double-conversion/strtod.cc)

vtk_module_add_module(VTK::doubleconversion
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkdoubleconversion")
vtk_module_include(VTK::doubleconversion
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/double-conversion>")

include(GenerateExportHeader)
generate_export_header(doubleconversion
  EXPORT_MACRO_NAME VTKDOUBLECONVERSION_EXPORT
  EXPORT_FILE_NAME double-conversion/vtkdoubleconversion_export.h)
