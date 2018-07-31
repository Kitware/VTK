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

#ensure we link against our dependencies
include(module.cmake)

vtk_module_dep_includes(vtkVTKm)
list(APPEND CMAKE_MODULE_PATH "${VTK_SOURCE_DIR}/ThirdParty/vtkm/vtk-m/CMake")

set(lib_srcs
  vtkmlib/PolyDataConverter.cxx
  vtkmlib/UnstructuredGridConverter.cxx
  vtkmlib/ArrayConverters.cxx
  vtkmlib/CellSetConverters.cxx
  vtkmlib/DataSetConverters.cxx
  vtkmlib/ImageDataConverter.cxx
  vtkmlib/Storage.cxx
  )

#needed to properly setup language wrappers
set(headers
  vtkmAverageToCells.h
  vtkmAverageToPoints.h
  vtkmCleanGrid.h
  vtkmClip.h
  vtkmContour.h
  vtkmExternalFaces.h
  vtkmExtractVOI.h
  vtkmGradient.h
  vtkmHistogram.h
  vtkmLevelOfDetail.h
  vtkmNDHistogram.h
  vtkmPointElevation.h
  vtkmPolyDataNormals.h
  vtkmProbe.h
  vtkmThreshold.h
  vtkmTriangleMeshPointNormals.h
  vtkmWarpScalar.h
  vtkmWarpVector.h
  )

#implementation of the algorithms for cpu accelerators
set(cpu_accelerator_srcs
  vtkmAverageToCells.cxx
  vtkmAverageToPoints.cxx
  vtkmCellSetExplicit.cxx
  vtkmCellSetSingleType.cxx
  vtkmCleanGrid.cxx
  vtkmClip.cxx
  vtkmConnectivityExec.cxx
  vtkmContour.cxx
  vtkmExternalFaces.cxx
  vtkmExtractVOI.cxx
  vtkmGradient.cxx
  vtkmHistogram.cxx
  vtkmLevelOfDetail.cxx
  vtkmNDHistogram.cxx
  vtkmPointElevation.cxx
  vtkmPolyDataNormals.cxx
  vtkmProbe.cxx
  vtkmThreshold.cxx
  vtkmTriangleMeshPointNormals.cxx
  vtkmWarpScalar.cxx
  vtkmWarpVector.cxx
  vtkmlib/Portals.cxx
  vtkmlib/ImplicitFunctionConverter.cxx
  )

#implementation of the algorithms for gpu accelerators
set(cuda_accelerator_srcs
  vtkmAverageToCells.cu
  vtkmAverageToPoints.cu
  vtkmCellSetExplicit.cu
  vtkmCellSetSingleType.cu
  vtkmCleanGrid.cu
  vtkmClip.cu
  vtkmConnectivityExec.cu
  vtkmContour.cu
  vtkmExternalFaces.cu
  vtkmExtractVOI.cu
  vtkmGradient.cu
  vtkmHistogram.cu
  vtkmLevelOfDetail.cu
  vtkmNDHistogram.cu
  vtkmPointElevation.cu
  vtkmPolyDataNormals.cu
  vtkmProbe.cu
  vtkmThreshold.cu
  vtkmTriangleMeshPointNormals.cu
  vtkmWarpScalar.cu
  vtkmWarpVector.cu
  vtkmlib/Portals.cu
  vtkmlib/ImplicitFunctionConverter.cu
  )

set(VTKM_FILTER_INCLUDE_AOS ${VTK_DISPATCH_AOS_ARRAYS})
set(VTKM_FILTER_INCLUDE_SOA ${VTK_DISPATCH_SOA_ARRAYS})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vtkmConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfig.h" @ONLY)

# ====================
# Important for wrapping and install rules for vtkmlib
#  - SKIP_HEADER_INSTALL is enabled as we need to install these headers
#    to the vtkmlib directory and not the root include directory as vtk
#    normally does
# ====================
set_source_files_properties(
  vtkmlib/PolyDataConverter
  vtkmlib/UnstructuredGridConverter
  vtkmlib/ArrayConverters
  vtkmlib/CellSetConverters
  vtkmlib/DataSetConverters
  vtkmlib/ImageDataConverter
  vtkmlib/Storage
  vtkmlib/Portals
  vtkmlib/ImplicitFunctionConverter
  PROPERTIES
    SKIP_HEADER_INSTALL 1
  )

set(${vtk-module}_HDRS
  vtkmTags.h
  vtkmFilterPolicy.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkmConfig.h
  )


# The VTK-m project uses large amounts of memory to compile as it does lots
# of template expansion. To reduce the amount of tension on the machine when
# using generators such as ninja we restrict the number of VTK-m enabled
# compilation units to be built at the same time.
# The logic we use is that no more than half the system memory should be used
# on VTK-m compilation units, where we expect each compilation unit to use
# 2GB of memory.
cmake_host_system_information(RESULT vtkm_mem_ QUERY TOTAL_PHYSICAL_MEMORY)
math(EXPR vtkm_pool_size "(${vtkm_mem_}/2)/2048")
if(vtkm_pool_size EQUAL 0)
  set(vtkm_pool_size 1)
endif()
set_property(GLOBAL APPEND PROPERTY JOB_POOLS vtkm_pool=${vtkm_pool_size})

if(TARGET vtkm::cuda)
  #we are building with CUDA support
  cmake_minimum_required(VERSION 3.9...3.12 FATAL_ERROR)
  enable_language(CUDA)

  # Workaround cmake issue #7519 which causes the c++11 flag set for VTK-m using
  # target_compile_features to not work.
  if (CMAKE_VERSION VERSION_LESS 3.11)
    set(CMAKE_CUDA_STANDARD 11)
    set(CMAKE_CUDA_STANDARD_REQUIRED True)
  endif()

  get_target_property(arch_flags vtkm::cuda VTKm_CUDA_Architecture_Flags)
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${arch_flags}")

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xnvlink -w")

  vtk_module_library(vtkAcceleratorsVTKm
                     ${headers}
                     ${lib_srcs}
                     ${cuda_accelerator_srcs}
                     )
  set_target_properties(vtkAcceleratorsVTKm PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
  target_compile_options(vtkAcceleratorsVTKm
    PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)
else()
  vtk_module_library(vtkAcceleratorsVTKm
                     ${headers}
                     ${lib_srcs}
                     ${cpu_accelerator_srcs}
                     )
endif()

set_property(TARGET vtkAcceleratorsVTKm PROPERTY JOB_POOL_COMPILE vtkm_pool)
target_link_libraries(vtkAcceleratorsVTKm PRIVATE vtkVTKm)

if (MSVC)
  #C4702 Generates numerous false positives with template code about
  #      unreachable code
  #C4512 Generates numerous warning that implicit assignment operators can't
  #      be constructed. This is understood and we don't care.
  #C4510 Generates numerous warning that implicit constructors can't
  #      be constructed. This is understood and we don't care.
  target_compile_options(vtkAcceleratorsVTKm PRIVATE -wd4702 -wd4512 -wd4510)
endif()



#install the required headers to make your own vtkm-vtk filter
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkmlib
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    FILES_MATCHING PATTERN "*.h*"
    )
endif()
