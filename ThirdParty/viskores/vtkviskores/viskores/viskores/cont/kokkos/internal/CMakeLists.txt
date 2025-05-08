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
  DeviceAdapterAlgorithmKokkos.h
  DeviceAdapterMemoryManagerKokkos.h
  DeviceAdapterRuntimeDetectorKokkos.h
  DeviceAdapterTagKokkos.h
  KokkosAlloc.h
  KokkosTypes.h
  RuntimeDeviceConfigurationKokkos.h
  )

viskores_declare_headers(${headers})

if (TARGET viskores_kokkos)
  set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmKokkos.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterMemoryManagerKokkos.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorKokkos.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/KokkosAlloc.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/KokkosTypes.cxx)
  target_sources(viskores_cont PRIVATE ${sources})

  if (TARGET viskores_kokkos_cuda)
    set_source_files_properties(${sources} TARGET_DIRECTORY viskores_cont PROPERTIES LANGUAGE CUDA)
  elseif(TARGET viskores_kokkos_hip)
    set_source_files_properties(${sources} TARGET_DIRECTORY viskores_cont PROPERTIES LANGUAGE HIP)
    kokkos_compilation(SOURCE ${sources})
    if (Viskores_ENABLE_KOKKOS_THRUST)
      # rocthrust does not wrap its compile defs/ops/dirs with $<$<COMPILE_LANGUAGE:HIP>.
      # We need this workaround since we mix CXX and HIP source files in viskores_cont.
      target_link_libraries(viskores_cont
        PRIVATE
          $<$<COMPILE_LANGUAGE:HIP>:roc::rocthrust>
          $<LINK_ONLY:roc::rocthrust>
      )
    endif()
  endif()

else()
  target_sources(viskores_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorKokkos.cxx)
endif()
