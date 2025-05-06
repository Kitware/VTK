##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  CudaAllocator.h
  DeviceAdapterAlgorithmCuda.h
  DeviceAdapterMemoryManagerCuda.h
  DeviceAdapterRuntimeDetectorCuda.h
  DeviceAdapterTagCuda.h
  DeviceAdapterTimerImplementationCuda.h
  MakeThrustIterator.h
  RuntimeDeviceConfigurationCuda.h
  ScopedCudaStackSize.h
  ThrustExceptionHandler.h
  )

viskores_declare_headers(${headers})


if (TARGET viskores_cuda)
  target_sources(viskores_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/CudaAllocator.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterMemoryManagerCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterTimerImplementationCuda.cu
    )
else()
#build the file with cpp compiler if cuda is disabled
  target_sources(viskores_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cxx
    )
endif()
