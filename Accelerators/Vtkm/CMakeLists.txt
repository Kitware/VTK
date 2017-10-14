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

vtk_module_dep_includes(vtkm)
list(APPEND CMAKE_MODULE_PATH "${VTK_SOURCE_DIR}/ThirdParty/vtkm/vtk-m/CMake")

if(VTKm_ENABLE_TBB)
  vtkm_configure_component_TBB()
endif()

if(VTKm_ENABLE_CUDA)
  vtkm_configure_component_CUDA()
endif()

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
  vtkmAverageToPoints.h
  vtkmCleanGrid.h
  vtkmClip.h
  vtkmContour.h
  vtkmExternalFaces.h
  vtkmExtractVOI.h
  vtkmThreshold.h
  vtkmLevelOfDetail.h
  vtkmAverageToCells.h
  vtkmGradient.h
  vtkmPolyDataNormals.h
  vtkmTriangleMeshPointNormals.h
  )

#implementation of the algorithms for cpu accelerators
set(cpu_accelerator_srcs
  vtkmAverageToPoints.cxx
  vtkmCleanGrid.cxx
  vtkmClip.cxx
  vtkmContour.cxx
  vtkmExternalFaces.cxx
  vtkmExtractVOI.cxx
  vtkmThreshold.cxx
  vtkmLevelOfDetail.cxx
  vtkmAverageToCells.cxx
  vtkmCellSetExplicit.cxx
  vtkmCellSetSingleType.cxx
  vtkmConnectivityExec.cxx
  vtkmGradient.cxx
  vtkmPolyDataNormals.cxx
  vtkmTriangleMeshPointNormals.cxx
  vtkmlib/Portals.cxx
  vtkmlib/ImplicitFunctionConverter.cxx
  )

#implementation of the algorithms for gpu accelerators
set(cuda_accelerator_srcs
  vtkmAverageToPoints.cu
  vtkmCleanGrid.cu
  vtkmClip.cu
  vtkmContour.cu
  vtkmExternalFaces.cu
  vtkmExtractVOI.cu
  vtkmThreshold.cu
  vtkmLevelOfDetail.cu
  vtkmAverageToCells.cu
  vtkmCellSetExplicit.cu
  vtkmCellSetSingleType.cu
  vtkmConnectivityExec.cu
  vtkmGradient.cu
  vtkmPolyDataNormals.cu
  vtkmTriangleMeshPointNormals.cu
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
#  - WRAP_EXCLUDE* are enabled as we don't want any vtkmlib private classes
#    being used as public api
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
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

# ====================
# Important for wrapping and install rules for vtkmlib
#  - WRAP_EXCLUDE* are enabled as we don't want any vtkm private classes
#    being used as public api
# ====================
set_source_files_properties(
  vtkmCellSetExplicit
  vtkmCellSetSingleType
  vtkmConnectivityExec
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )


set(${vtk-module}_HDRS
  vtkmTags.h
  vtkmFilterPolicy.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkmConfig.h
  )

#we are building with CUDA support
if(VTKm_ENABLE_CUDA)

  #need to find cudadevrt
  find_library(CUDA_cudadevrt_LIBRARY cudadevrt
               PATHS ${CUDA_TOOLKIT_TARGET_DIR}
               PATH_SUFFIXES "x64" "lib64" "libx64"
               )

  ########
  ## cache and clear the CUDA_NVCC_FLAGS so that they aren't passed to
  ## the linker. FINDCUDA has some problems properly unquoting CUDA_NVCC_FLAGS
  ## when "generate-code arch..." is used, so we instead patch the options
  ##
  ########
  set(compile_options)
  foreach(f ${CUDA_NVCC_FLAGS})
    if(f MATCHES "generate-code ")
      string(REPLACE "generate-code " "generate-code=" f "${f}")
    endif()
    list(APPEND compile_options ${f})
  endforeach()

  if(BUILD_SHARED_LIBS AND NOT WIN32)
    list(APPEND compile_options -Xcompiler=${CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY}hidden)
    list(APPEND compile_options -Xcompiler=-fPIC)
    #nvcc doesn't like the macros in VTK and generates hundreds of warnings
    #that are false positives
    list(APPEND compile_options --disable-warnings)
  endif()

  set(seperable_state ${CUDA_SEPARABLE_COMPILATION})
  set(cache_flag_state ${CUDA_NVCC_FLAGS})


  set(CUDA_NVCC_FLAGS "")
  set(CUDA_SEPARABLE_COMPILATION ON)

  #Some versions of VTK-m overload the CUDA_LIBRARIES to contain private
  if(PRIVATE IN_LIST CUDA_LIBRARIES)
    set(cache_cuda_libs ${CUDA_LIBRARIES})
    set(cache_devrt_libs ${CUDA_cudadevrt_LIBRARY})
    set(CUDA_LIBRARIES ${CUDA_LIBRARIES} ${vtkAcceleratorsVTKm_DEPENDS})
    set(CUDA_cudadevrt_LIBRARY PRIVATE ${CUDA_cudadevrt_LIBRARY})
  endif()

  # CUDA doesn't obey usage requirements so we have to use
  # CUDA_INCLUDE_DIRECTORIES, but do get the proper list of
  # include dirs I need to query the module system, which
  # doesnt exist currently, so we manually call vtk_module_impl
  vtk_module_impl()
  cuda_include_directories(${CMAKE_CURRENT_BINARY_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${VTKm_INCLUDE_DIRS}
                           ${vtkAcceleratorsVTKm_DEPENDS_INCLUDE_DIRS})

  cuda_add_library(vtkAcceleratorsVTKmCuda STATIC
                   ${cuda_accelerator_srcs}
                   OPTIONS "${compile_options}"
                   )

  set_target_properties(vtkAcceleratorsVTKmCuda
                        PROPERTIES POSITION_INDEPENDENT_CODE True)

  vtk_module_library(vtkAcceleratorsVTKm
                     ${headers}
                     ${lib_srcs}
                     )

  target_link_libraries(vtkAcceleratorsVTKm
                        PRIVATE vtkAcceleratorsVTKmCuda ${cache_devrt_libs})

  set(CUDA_SEPARABLE_COMPILATION ${seperable_state})
  set(CUDA_NVCC_FLAGS_CACHE  ${cache_flag_state})

  if(cache_cuda_libs)
    set(CUDA_LIBRARIES  ${cache_cuda_libs})
    set(CUDA_cudadevrt_LIBRARY  ${CUDA_cudadevrt_LIBRARY})
  endif()
else()
  vtk_module_library(vtkAcceleratorsVTKm
                     ${headers}
                     ${lib_srcs}
                     ${cpu_accelerator_srcs}
                     )
endif()

target_link_libraries(vtkAcceleratorsVTKm PRIVATE ${VTKm_LIBRARIES})
target_include_directories(vtkAcceleratorsVTKm PRIVATE ${VTKm_INCLUDE_DIRS})

if (MSVC)
  # MSVC has limits on obj size that are exceeded by this module so we need
  # a flag to ignore those limits
  target_compile_options(vtkAcceleratorsVTKm PRIVATE /bigobj)

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
