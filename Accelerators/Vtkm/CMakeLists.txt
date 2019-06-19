##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2012 Sandia Corporation.
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##=============================================================================

list(INSERT 0 CMAKE_MODULE_PATH
  "${VTK_SOURCE_DIR}/ThirdParty/vtkm/vtkvtkm/vtk-m/CMake")

set(sources
  vtkmlib/ArrayConvertersReal.cxx
  vtkmlib/ArrayConvertersSigned.cxx
  vtkmlib/ArrayConvertersUnsigned.cxx)

set(headers
  vtkmFilterPolicy.h
  vtkmTags.h)

set(private_headers
  vtkmlib/ArrayConverters.hxx
  vtkmlib/DataSetConverters.h
  vtkmlib/Portals.hxx
  vtkmlib/PortalTraits.h
  vtkmlib/Storage.hxx
  vtkmDataArray.hxx)

set(impl_classes
  vtkmAverageToCells
  vtkmAverageToPoints
  vtkmCellSetExplicit
  vtkmCellSetSingleType
  vtkmCleanGrid
  vtkmClip
  vtkmConnectivityExec
  vtkmContour
  vtkmCoordinateSystemTransform
  vtkmDataArray
  vtkmDataSet
  vtkmExternalFaces
  vtkmExtractVOI
  vtkmGradient
  vtkmHistogram
  vtkmLevelOfDetail
  vtkmNDHistogram
  vtkmPointElevation
  vtkmPointTransform
  vtkmPolyDataNormals
  vtkmProbe
  vtkmThreshold
  vtkmTriangleMeshPointNormals
  vtkmWarpScalar
  vtkmWarpVector
  )

set(private_impl_classes
  vtkmlib/ArrayConverters
  vtkmlib/CellSetConverters
  vtkmlib/DataSetConverters
  vtkmlib/ImageDataConverter
  vtkmlib/ImplicitFunctionConverter
  vtkmlib/PolyDataConverter
  vtkmlib/Portals
  vtkmlib/Storage
  vtkmlib/UnstructuredGridConverter)

foreach (class IN LISTS impl_classes)
  list(APPEND sources "${class}.cxx")
  list(APPEND headers "${class}.h")
endforeach ()

foreach (class IN LISTS private_impl_classes)
  list(APPEND sources "${class}.cxx")
  list(APPEND private_headers "${class}.h")
endforeach ()

if (TARGET vtkm::cuda)
  set_source_files_properties(${sources} PROPERTIES LANGUAGE "CUDA")
endif ()

set(VTKM_FILTER_INCLUDE_AOS ${VTK_DISPATCH_AOS_ARRAYS})
set(VTKM_FILTER_INCLUDE_SOA ${VTK_DISPATCH_SOA_ARRAYS})
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfig.h"
  @ONLY)

list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfig.h")

# The VTK-m project uses large amounts of memory to compile as it does lots
# of template expansion. To reduce the amount of tension on the machine when
# using generators such as ninja we restrict the number of VTK-m enabled
# compilation units to be built at the same time.
# The logic we use is that no more than half the system memory should be used
# on VTK-m compilation units, where we expect each compilation unit to use
# 2GB of memory.
cmake_host_system_information(RESULT vtkm_mem_ QUERY TOTAL_PHYSICAL_MEMORY)
math(EXPR vtkm_pool_size "(${vtkm_mem_}/2)/2048")
if (vtkm_pool_size EQUAL 0)
  set(vtkm_pool_size 1)
endif ()
set_property(GLOBAL APPEND
  PROPERTY
    JOB_POOLS vtkm_pool=${vtkm_pool_size})

if (TARGET vtkm::cuda)
  enable_language(CUDA)

  # XXX(cmake#7519): Workaround for CMake which causes the c++11 flag set for
  # VTK-m using target_compile_features to not work.
  if (CMAKE_VERSION VERSION_LESS 3.11)
    set(CMAKE_CUDA_STANDARD 11)
    set(CMAKE_CUDA_STANDARD_REQUIRED True)
  endif()

  get_property(arch_flags
    TARGET    vtkm::cuda
    PROPERTY  INTERFACE_CUDA_Architecture_Flags)
  if(NOT arch_flags)
    message(FATAL_ERROR "VTK-m CUDA Architecture flags can't be found. This means we can't safely compile CUDA code")
  endif()
  string(APPEND CMAKE_CUDA_FLAGS " ${arch_flags}")

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif ()

if(TARGET vtkm::cuda AND BUILD_SHARED_LIBS)
  message("Cuda is enabled. Module VTK::AcceleratorsVTKm will be forced to build as static.")
  vtk_module_add_module(VTK::AcceleratorsVTKm
    FORCE_STATIC
    SOURCES ${sources}
    HEADERS ${headers}
    PRIVATE_HEADERS ${private_headers})
else()
  vtk_module_add_module(VTK::AcceleratorsVTKm
    SOURCES ${sources}
    HEADERS ${headers}
    PRIVATE_HEADERS ${private_headers})
endif()

vtk_module_set_property(VTK::AcceleratorsVTKm
  PROPERTY  JOB_POOL_COMPILE
  VALUE     vtkm_pool)
vtk_module_link(VTK::AcceleratorsVTKm
  PRIVATE
    vtkm_worklet)

if (TARGET vtkm::cuda)
  vtk_module_set_property(VTK::AcceleratorsVTKm
    PROPERTY  CUDA_SEPARABLE_COMPILATION
    VALUE     ON)
  vtk_module_compile_options(VTK::AcceleratorsVTKm
    PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)
endif ()

# We need to install the private headers so that consumers of VTK can write
# new VTK-m filters and do data conversions correctly.
vtk_module_install_headers(
    FILES   ${private_headers}
    SUBDIR  "vtkmlib")

if (MSVC)
  set(msvc_warning_flags
    # Generates numerous false positives with template code about unreachable
    # code
    /wd4702
    # Generates numerous warning that implicit assignment operators can't be
    # constructed. This is understood and we don't care.
    /wd4512
    # Generates numerous warning that implicit constructors can't be
    # constructed. This is understood and we don't care.
    /wd4510)
  set(vtkm_msvc_flags)
  foreach (msvc_warning_flag IN LISTS msvc_warning_flags)
    if (TARGET vtkm::cuda)
      list(APPEND vtkm_msvc_flags
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${msvc_warning_flag},${msvc_warning_flag}>)
    else ()
      list(APPEND vtkm_msvc_flags
        ${msvc_warning_flag})
    endif ()
  endforeach ()
  vtk_module_compile_options(VTK::AcceleratorsVTKm
    PRIVATE
      ${msvc_warning_flags})
endif ()
