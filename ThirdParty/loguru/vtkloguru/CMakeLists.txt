set(headers
  loguru.hpp
  "${CMAKE_CURRENT_BINARY_DIR}/vtkloguru_export.h")

set(sources
  loguru.cpp)

include(CheckIncludeFile)
check_include_file("<cxxabi.h>" VTK_LOGURU_HAVE_CXXABI_H)
check_include_file("<dlfcn.h>" VTK_LOGURU_HAVE_DLFCN_H)
check_include_file("<execinfo.h>" VTK_LOGURU_HAVE_EXECINFO_H)

set(loguru_have_stacktraces 0)
if (VTK_LOGURU_HAVE_CXXABI_H AND VTK_LOGURU_HAVE_DLFCN_H AND VTK_LOGURU_HAVE_EXECINFO_H)
  set(loguru_have_stacktraces 1)
endif ()

vtk_module_add_module(VTK::loguru
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkloguru")
vtk_module_definitions(VTK::loguru
  PRIVATE
    "LOGURU_SCOPE_TIME_PRECISION=${VTK_LOGGING_TIME_PRECISION}"
    "LOGURU_STACKTRACES=$<BOOL:${loguru_have_stacktraces}>")
# Loguru dynamically sets LOGURU_PTLS_NAMES = 1 in Linux (`__linux__`)
# Additionally, loguru also checks if user has defined LOGURU_PTLS_NAMES and avoids using pthread_[set]getname_np
# wasm currently doesn't implement pthread_[set]getname_np, so ask to use TLS for thread names.
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_definitions(VTK::loguru
    PRIVATE
      "LOGURU_PTLS_NAMES=1")
endif ()

include(GenerateExportHeader)
generate_export_header(loguru
  EXPORT_MACRO_NAME LOGURU_EXPORT
  EXPORT_FILE_NAME vtkloguru_export.h)

vtk_module_find_package(PACKAGE Threads)
vtk_module_link(VTK::loguru PRIVATE Threads::Threads)

if (loguru_have_stacktraces)
  vtk_module_link(VTK::loguru PRIVATE ${CMAKE_DL_LIBS})
endif ()
