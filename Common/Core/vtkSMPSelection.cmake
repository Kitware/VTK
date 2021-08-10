set(VTK_SMP_IMPLEMENTATION_TYPE "Sequential"
  CACHE STRING "Which multi-threaded parallelism implementation to use. Options are Sequential, STDThread, OpenMP or TBB")
set_property(CACHE VTK_SMP_IMPLEMENTATION_TYPE
  PROPERTY
    STRINGS Sequential OpenMP TBB STDThread)

if (NOT (VTK_SMP_IMPLEMENTATION_TYPE STREQUAL "OpenMP" OR
         VTK_SMP_IMPLEMENTATION_TYPE STREQUAL "TBB" OR
         VTK_SMP_IMPLEMENTATION_TYPE STREQUAL "Sequential"))
  set_property(CACHE VTK_SMP_IMPLEMENTATION_TYPE
    PROPERTY
      VALUE "STDThread")
endif ()

option(VTK_SMP_ENABLE_SEQUENTIAL "Enable Sequential backend" ON)
option(VTK_SMP_ENABLE_STDTHREAD "Enable STDThread backend" ON)

# Force option to be ON if VTK_SMP_IMPLEMENTATION_TYPE equal to the backend otherwise it's OFF by default
cmake_dependent_option(VTK_SMP_ENABLE_TBB "Enable TBB backend" OFF
  "NOT VTK_SMP_IMPLEMENTATION_TYPE STREQUAL TBB" ON)
cmake_dependent_option(VTK_SMP_ENABLE_OPENMP "Enable OpenMP backend" OFF
  "NOT VTK_SMP_IMPLEMENTATION_TYPE STREQUAL OpenMP" ON)

mark_as_advanced(
  VTK_SMP_ENABLE_SEQUENTIAL
  VTK_SMP_ENABLE_STDTHREAD
  VTK_SMP_ENABLE_TBB
  VTK_SMP_ENABLE_OPENMP)

set(vtk_smp_default_name)
string(TOUPPER "${VTK_SMP_IMPLEMENTATION_TYPE}" vtk_smp_default_name)
if (NOT VTK_SMP_ENABLE_${vtk_smp_default_name})
  message(FATAL_ERROR "VTK_SMP_ENABLE_${vtk_smp_default_name} must be enabled as VTK_SMP_IMPLEMENTATION_TYPE is set to ${VTK_SMP_IMPLEMENTATION_TYPE}.")
endif()

set(vtk_smp_defines)
set(vtk_smp_use_default_atomics ON)

set(vtk_smp_backends)

if (VTK_SMP_ENABLE_TBB)
  vtk_module_find_package(PACKAGE TBB)
  list(APPEND vtk_smp_libraries
    TBB::tbb)
  set(vtk_smp_enable_tbb 1)
  list(APPEND vtk_smp_backends "TBB")

  set(vtk_smp_use_default_atomics OFF)
  set(vtk_smp_implementation_dir SMP/TBB)
  list(APPEND vtk_smp_sources
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.cxx")
  list(APPEND vtk_smp_nowrap_headers
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalImpl.h")
  list(APPEND vtk_smp_templates
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.txx")
endif()

if (VTK_SMP_ENABLE_OPENMP)
  vtk_module_find_package(PACKAGE OpenMP)
  list(APPEND vtk_smp_libraries
    OpenMP::OpenMP_CXX)
  set(vtk_smp_enable_openmp 1)
  list(APPEND vtk_smp_backends "OpenMP")

  set(vtk_smp_implementation_dir SMP/OpenMP)
  list(APPEND vtk_smp_sources
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.cxx"
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalBackend.cxx")
  list(APPEND vtk_smp_nowrap_headers
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalImpl.h"
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalBackend.h")
  list(APPEND vtk_smp_templates
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.txx")

  if (OpenMP_CXX_SPEC_DATE AND NOT "${OpenMP_CXX_SPEC_DATE}" LESS "201107")
    set(vtk_smp_use_default_atomics OFF)
  else()
    message(WARNING
      "Required OpenMP version (3.1) for atomics not detected. Using default "
      "atomics implementation.")
  endif()
endif()

if (VTK_SMP_ENABLE_STDTHREAD)
  set(vtk_smp_enable_stdthread 1)
  set(vtk_smp_implementation_dir SMP/STDThread)
  list(APPEND vtk_smp_backends "STDThread")

  list(APPEND vtk_smp_sources
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.cxx"
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalBackend.cxx"
    "${vtk_smp_implementation_dir}/vtkSMPThreadPool.cxx")
  list(APPEND vtk_smp_nowrap_headers
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalImpl.h"
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalBackend.h"
    "${vtk_smp_implementation_dir}/vtkSMPThreadPool.h")
  list(APPEND vtk_smp_templates
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.txx")
endif()

if (VTK_SMP_ENABLE_SEQUENTIAL)
  set(vtk_smp_enable_sequential 1)
  set(vtk_smp_implementation_dir SMP/Sequential)
  list(APPEND vtk_smp_backends "Sequential")

  list(APPEND vtk_smp_sources
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.cxx")
  list(APPEND vtk_smp_nowrap_headers
    "${vtk_smp_implementation_dir}/vtkSMPThreadLocalImpl.h")
  list(APPEND vtk_smp_templates
    "${vtk_smp_implementation_dir}/vtkSMPToolsImpl.txx")
endif()

set_property(GLOBAL
  PROPERTY
    _vtk_smp_backends "${vtk_smp_backends}")

if (vtk_smp_use_default_atomics)
  include(CheckSymbolExists)

  include("${CMAKE_CURRENT_SOURCE_DIR}/vtkTestBuiltins.cmake")

  set(vtk_atomics_default_impl_dir "${CMAKE_CURRENT_SOURCE_DIR}/SMP/Sequential")
endif()

set(vtk_smp_common_dir SMP/Common)
list(APPEND vtk_smp_sources
  "${vtk_smp_common_dir}/vtkSMPToolsAPI.cxx")
list(APPEND vtk_smp_nowrap_headers
  "${vtk_smp_common_dir}/vtkSMPThreadLocalAPI.h"
  "${vtk_smp_common_dir}/vtkSMPThreadLocalImplAbstract.h"
  "${vtk_smp_common_dir}/vtkSMPToolsAPI.h"
  "${vtk_smp_common_dir}/vtkSMPToolsImpl.h"
  "${vtk_smp_common_dir}/vtkSMPToolsInternal.h")

list(APPEND vtk_smp_sources
  vtkSMPTools.cxx)
list(APPEND vtk_smp_headers
  vtkSMPTools.h
  vtkSMPThreadLocal.h
  vtkSMPThreadLocalObject.h)
