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

set(VTKm_NO_DEPRECATED_VIRTUAL ON)

list(INSERT CMAKE_MODULE_PATH 0
  "${VTK_SOURCE_DIR}/ThirdParty/vtkm/vtkvtkm/vtk-m/CMake")

set(sources
  vtkmlib/ArrayConvertersReal.cxx
  vtkmlib/ArrayConvertersSigned.cxx
  vtkmlib/ArrayConvertersUnsigned.cxx)

set(private_headers
  vtkmFilterPolicy.h
  vtkmlib/ArrayConverters.hxx)

set(classes
  vtkmDataSet
  )

set(nowrap_classes
  vtkmlib/ArrayConverters
  vtkmlib/CellSetConverters
  vtkmlib/DataSetConverters
  vtkmlib/ImageDataConverter
  vtkmlib/ImplicitFunctionConverter
  vtkmlib/PolyDataConverter
  vtkmlib/UnstructuredGridConverter)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmConfigDataModel.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigDataModel.h"
  @ONLY)

list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigDataModel.h")

if (TARGET vtkm::cuda)
  enable_language(CUDA)

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif ()

vtk_module_add_module(VTK::AcceleratorsVTKmDataModel
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  CLASSES ${classes}
  NOWRAP_CLASSES  ${nowrap_classes}
  PRIVATE_HEADERS ${private_headers})
_vtk_module_real_target(vtkm_accel_target VTK::AcceleratorsVTKmDataModel)
vtkm_add_target_information(${vtkm_accel_target}
                            EXTENDS_VTKM
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES ${sources})

vtk_module_set_property(VTK::AcceleratorsVTKmDataModel
  PROPERTY  JOB_POOL_COMPILE
  VALUE     vtkm_pool)

if (TARGET vtkm::cuda)
  set(cuda_impl ${nowrap_classes} vtkmDataSet.cxx)
  set_source_files_properties(${cuda_impl} PROPERTIES
    LANGUAGE CUDA
    CUDA_SOURCE_PROPERTY_FORMAT OBJ
    CUDA_SEPARABLE_COMPILATION ON)

  vtk_module_set_properties(VTK::AcceleratorsVTKmDataModel
    LANGUAGE CUDA
    CUDA_SEPARABLE_COMPILATION ON)

  vtk_module_compile_options(VTK::AcceleratorsVTKmDataModel
    PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)
endif ()

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
    /wd4510
    # Generates warnings when decorated names are over 4096 characters
    # This compiler warning is deprecated and no longer occurs with VS2017+.
    # So we suppress for VS2015
    /wd4503
    )
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
  vtk_module_compile_options(VTK::AcceleratorsVTKmDataModel
    PRIVATE
      ${vtkm_msvc_flags})
endif ()
